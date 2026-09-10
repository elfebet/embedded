(function(){
  "use strict";

  // ---------- theme toggle ----------
  var root = document.documentElement;
  var themeBtn = document.getElementById('themeToggle');
  themeBtn.addEventListener('click', function(){
    var cur = root.getAttribute('data-theme');
    var prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    var next;
    if (!cur) next = prefersDark ? 'light' : 'dark';
    else next = cur === 'dark' ? 'light' : 'dark';
    root.setAttribute('data-theme', next);
  });

  // ---------- clock (from telemetry, ticks locally between messages) ----------
  var clockEl = document.getElementById('clock');
  var lastUtc = null, lastUtcAt = 0;
  function tickClock(){
    if (!lastUtc) return;
    var parts = lastUtc.split(':').map(Number);
    if (parts.length !== 3 || parts.some(isNaN)) { clockEl.textContent = lastUtc; return; }
    var elapsed = Math.floor((Date.now() - lastUtcAt) / 1000);
    var total = parts[0]*3600 + parts[1]*60 + parts[2] + elapsed;
    total = ((total % 86400) + 86400) % 86400;
    var hh = String(Math.floor(total/3600)).padStart(2,'0');
    var mm = String(Math.floor((total%3600)/60)).padStart(2,'0');
    var ss = String(total%60).padStart(2,'0');
    clockEl.textContent = hh+':'+mm+':'+ss;
  }
  setInterval(tickClock, 1000);

  // ================= minimal MQTT 3.1.1 client over WebSocket =================
  function encodeUTF8String(str){
    var bytes = new TextEncoder().encode(str);
    var out = new Uint8Array(2 + bytes.length);
    out[0] = (bytes.length >> 8) & 0xFF;
    out[1] = bytes.length & 0xFF;
    out.set(bytes, 2);
    return out;
  }
  function encodeRemainingLength(len){
    var bytes = [];
    do {
      var b = len % 128;
      len = Math.floor(len / 128);
      if (len > 0) b |= 0x80;
      bytes.push(b);
    } while (len > 0);
    return bytes;
  }
  function readRemainingLength(buf, offset){
    var multiplier = 1, value = 0, i = offset, used = 0, byte;
    while (true){
      if (i >= buf.length) return null;
      byte = buf[i]; i++; used++;
      value += (byte & 0x7F) * multiplier;
      if ((byte & 0x80) === 0) break;
      multiplier *= 128;
      if (multiplier > 128*128*128) throw new Error('malformed remaining length');
    }
    return { value: value, bytesUsed: used };
  }
  function concatAll(arrays){
    var total = 0;
    for (var i=0;i<arrays.length;i++) total += arrays[i].length;
    var out = new Uint8Array(total);
    var off = 0;
    for (var j=0;j<arrays.length;j++){ out.set(arrays[j], off); off += arrays[j].length; }
    return out;
  }
  function u8(arr){ return new Uint8Array(arr); }

  function buildConnect(clientId, keepAlive){
    var protoName = encodeUTF8String('MQTT');
    var protoLevel = u8([4]);
    var connectFlags = u8([0x02]); // clean session, no will/user/pass
    var keepAliveBytes = u8([(keepAlive>>8)&0xFF, keepAlive & 0xFF]);
    var clientIdBytes = encodeUTF8String(clientId);
    var vhp = concatAll([protoName, protoLevel, connectFlags, keepAliveBytes, clientIdBytes]);
    var rl = encodeRemainingLength(vhp.length);
    return concatAll([u8([0x10].concat(rl)), vhp]);
  }
  function buildSubscribe(packetId, topic, qos){
    var pid = u8([(packetId>>8)&0xFF, packetId & 0xFF]);
    var topicBytes = encodeUTF8String(topic);
    var vhp = concatAll([pid, topicBytes, u8([qos])]);
    var rl = encodeRemainingLength(vhp.length);
    return concatAll([u8([0x82].concat(rl)), vhp]);
  }
  function buildPublish(topic, payload, retain){
    var topicBytes = encodeUTF8String(topic);
    var payloadBytes = new TextEncoder().encode(payload);
    var vhp = concatAll([topicBytes, payloadBytes]);
    var rl = encodeRemainingLength(vhp.length);
    var flags = 0x30 | (retain ? 0x01 : 0x00);
    return concatAll([u8([flags].concat(rl)), vhp]);
  }
  var PINGREQ = u8([0xC0, 0x00]);
  var DISCONNECT = u8([0xE0, 0x00]);

  function tryParsePacket(buf){
    if (buf.length < 2) return null;
    var rl = readRemainingLength(buf, 1);
    if (!rl) return null;
    var total = 1 + rl.bytesUsed + rl.value;
    if (buf.length < total) return null;
    return { packet: buf.slice(0, total), total: total, rlBytesUsed: rl.bytesUsed };
  }
  function parsePublish(packet, rlBytesUsed){
    var idx = 1 + rlBytesUsed;
    var topicLen = (packet[idx]<<8) | packet[idx+1]; idx += 2;
    var topic = new TextDecoder().decode(packet.slice(idx, idx+topicLen)); idx += topicLen;
    var qos = (packet[0] >> 1) & 0x3;
    if (qos > 0) idx += 2; // packet id, not expected at our QoS0 subscription
    var payload = new TextDecoder().decode(packet.slice(idx));
    return { topic: topic, payload: payload };
  }

  // ---------- link state ----------
  var ws = null, rxBuffer = new Uint8Array(0), pingTimer = null, reconnectTimer = null;
  var manualDisconnect = false, connected = false, nextPid = 1;
  var KEEP_ALIVE = 60;

  var statusChip = document.getElementById('statusChip');
  var statusText = document.getElementById('statusText');
  var linkBtn = document.getElementById('linkBtn');
  var brokerInput = document.getElementById('brokerUrl');
  var clientIdInput = document.getElementById('clientId');
  var autoReconnectChk = document.getElementById('autoReconnect');

  function setStatus(state, text){
    statusChip.setAttribute('data-state', state);
    statusText.textContent = text;
  }

  function randomClientId(){
    return 'webdash-' + Math.random().toString(16).slice(2, 8);
  }
  try {
    brokerInput.value = localStorage.getItem('iotlogger_broker') || brokerInput.value;
    clientIdInput.value = localStorage.getItem('iotlogger_client') || randomClientId();
  } catch(e){ clientIdInput.value = randomClientId(); }

  function connectMqtt(){
    manualDisconnect = false;
    var url = brokerInput.value.trim();
    var clientId = clientIdInput.value.trim() || randomClientId();
    try { localStorage.setItem('iotlogger_broker', url); localStorage.setItem('iotlogger_client', clientId); } catch(e){}

    setStatus('connecting', 'LINKING...');
    linkBtn.disabled = true;
    rxBuffer = new Uint8Array(0);

    try {
      ws = new WebSocket(url, 'mqtt');
    } catch (e) {
      // Two very different causes land here, so don't lump them into one
      // generic label: a malformed URL throws SyntaxError; opening ws://
      // from a page loaded over https:// throws SecurityError (mixed
      // content) even when the URL itself is perfectly correct.
      var isMixedContent = (window.location.protocol === 'https:' && /^ws:\/\//i.test(url));
      if (isMixedContent) {
        setStatus('offline', 'BLOCKED (https page, use ws:// only from file:// or http://)');
      } else if (e && e.name === 'SyntaxError') {
        setStatus('offline', 'BAD URL FORMAT');
      } else {
        setStatus('offline', 'CONNECT ERROR');
      }
      linkBtn.disabled = false;
      return;
    }
    ws.binaryType = 'arraybuffer';

    ws.onopen = function(){
      ws.send(buildConnect(clientId, KEEP_ALIVE));
    };
    ws.onmessage = function(ev){
      var incoming = new Uint8Array(ev.data);
      var merged = new Uint8Array(rxBuffer.length + incoming.length);
      merged.set(rxBuffer, 0); merged.set(incoming, rxBuffer.length);
      rxBuffer = merged;
      while (true){
        var res;
        try { res = tryParsePacket(rxBuffer); } catch(e){ rxBuffer = new Uint8Array(0); break; }
        if (!res) break;
        handlePacket(res.packet, res.rlBytesUsed);
        rxBuffer = rxBuffer.slice(res.total);
      }
    };
    ws.onclose = function(){
      connected = false;
      linkBtn.disabled = false;
      linkBtn.textContent = 'LINK';
      setStatus('offline', 'OFFLINE');
      if (pingTimer) { clearInterval(pingTimer); pingTimer = null; }
      if (!manualDisconnect && autoReconnectChk.checked){
        reconnectTimer = setTimeout(connectMqtt, 3000);
      }
    };
    ws.onerror = function(){ /* onclose follows */ };
  }

  function handlePacket(packet, rlBytesUsed){
    var type = packet[0] >> 4;
    if (type === 2) { // CONNACK
      var returnCode = packet[3];
      if (returnCode === 0){
        connected = true;
        linkBtn.disabled = false;
        linkBtn.textContent = 'UNLINK';
        setStatus('linked', 'LINKED');
        subscribe('logger/telemetry', 0);
        if (pingTimer) clearInterval(pingTimer);
        pingTimer = setInterval(function(){ if (ws && ws.readyState === 1) ws.send(PINGREQ); }, 20000);
      } else {
        setStatus('offline', 'REFUSED ('+returnCode+')');
        linkBtn.disabled = false;
      }
    } else if (type === 3) { // PUBLISH
      var msg = parsePublish(packet, rlBytesUsed);
      onMessage(msg.topic, msg.payload);
    }
    // SUBACK(9) / PINGRESP(13): no action needed
  }

  function subscribe(topic, qos){
    var pid = nextPid; nextPid = (nextPid % 65535) + 1;
    ws.send(buildSubscribe(pid, topic, qos));
  }
  function publish(topic, payload){
    if (!connected || !ws || ws.readyState !== 1) return false;
    ws.send(buildPublish(topic, payload, false));
    return true;
  }

  linkBtn.addEventListener('click', function(){
    if (connected || (ws && ws.readyState === WebSocket.CONNECTING)){
      manualDisconnect = true;
      if (reconnectTimer) { clearTimeout(reconnectTimer); reconnectTimer = null; }
      try { if (ws.readyState === 1) ws.send(DISCONNECT); } catch(e){}
      try { ws.close(); } catch(e){}
    } else {
      connectMqtt();
    }
  });

  // ---------- UI wiring for telemetry ----------
  var msgCount = 0;
  var feedLog = document.getElementById('feedLog');
  var feedCount = document.getElementById('feedCount');

  function logFeed(text){
    if (feedLog.querySelector('.empty')) feedLog.innerHTML = '';
    var line = document.createElement('div');
    var t = new Date();
    var ts = String(t.getHours()).padStart(2,'0')+':'+String(t.getMinutes()).padStart(2,'0')+':'+String(t.getSeconds()).padStart(2,'0');
    var tSpan = document.createElement('span'); tSpan.className='t'; tSpan.textContent = '['+ts+'] ';
    line.appendChild(tSpan);
    line.appendChild(document.createTextNode(text));
    feedLog.appendChild(line);
    while (feedLog.children.length > 40) feedLog.removeChild(feedLog.firstChild);
    feedLog.scrollTop = feedLog.scrollHeight;
    msgCount++; feedCount.textContent = msgCount + ' MSG';
  }

  function setSensorTile(prefix, ok, temp, hum, pres){
    var pill = document.getElementById('pill'+prefix);
    var tile = document.getElementById('tile'+prefix);
    var tempEl = document.getElementById('temp'+prefix);
    var humEl = document.getElementById('hum'+prefix);
    var presEl = document.getElementById('pres'+prefix);
    if (ok){
      tile.classList.remove('fault');
      pill.setAttribute('data-ok','true'); pill.textContent = 'OK';
      tempEl.textContent = temp.toFixed(1);
      humEl.textContent = hum.toFixed(0) + ' %RH';
      presEl.textContent = pres.toFixed(0) + ' hPa';
    } else {
      tile.classList.add('fault');
      pill.setAttribute('data-ok','false'); pill.textContent = 'FAULT';
      tempEl.textContent = '--';
      humEl.textContent = '-- %RH';
      presEl.textContent = '-- hPa';
    }
  }

  var staleTimer = null;
  function armStaleWatch(){
    if (staleTimer) clearTimeout(staleTimer);
    staleTimer = setTimeout(function(){
      ['I2C','SPI'].forEach(function(p){
        var pill = document.getElementById('pill'+p);
        if (pill.getAttribute('data-ok') !== 'false') pill.setAttribute('data-ok','stale');
        pill.textContent = 'STALE';
      });
    }, 7000);
  }

  function onMessage(topic, payload){
    if (topic !== 'logger/telemetry') return;
    var data;
    try { data = JSON.parse(payload); } catch(e){ logFeed('RX malformed payload: '+payload); return; }

    logFeed(payload);
    armStaleWatch();

    if (data.utc_time){ lastUtc = data.utc_time; lastUtcAt = Date.now(); tickClock(); }

    setSensorTile('I2C', !!data.i2c_ok, Number(data.temp_i2c_c), Number(data.hum_i2c_rh), Number(data.press_i2c_hpa));
    setSensorTile('SPI', !!data.spi_ok, Number(data.temp_spi_c), Number(data.hum_spi_rh), Number(data.press_spi_hpa));

    if (typeof data.threshold_c === 'number'){
      var pct = Math.max(0, Math.min(100, (data.threshold_c - 15) / (35 - 15) * 100));
      document.getElementById('gaugeFill').style.width = pct + '%';
      document.getElementById('pillThresh').textContent = data.threshold_c.toFixed(0) + '°C';
      document.getElementById('pillThresh').setAttribute('data-ok', 'true');
    }

    if (typeof data.adc_raw === 'number') {
      document.getElementById('adcRaw').textContent = data.adc_raw + ' / 4095';
    }

    var ledOn = !!data.led_on;
    document.getElementById('ledLamp').setAttribute('data-on', ledOn ? 'true':'false');
    document.getElementById('ledState').textContent = ledOn ? 'ON' : 'OFF';
    document.getElementById('ledSource').textContent = data.led_source ? String(data.led_source).toUpperCase() : '--';
  }

  document.getElementById('ledOnBtn').addEventListener('click', function(){
    if (publish('logger/control/led', 'ON')) logFeed('TX logger/control/led = ON');
  });
  document.getElementById('ledOffBtn').addEventListener('click', function(){
    if (publish('logger/control/led', 'OFF')) logFeed('TX logger/control/led = OFF');
  });

})();
