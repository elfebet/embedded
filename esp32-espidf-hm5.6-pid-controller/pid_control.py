import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv('/Users/anton/Desktop/csv6.txt')

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

# --- ВЕРХНІЙ ГРАФІК: Setpoint vs PV ---
ax1.plot(df['t_ms'], df['setpoint'], label='Setpoint', color='orange', linestyle='--', linewidth=2)
ax1.plot(df['t_ms'], df['current'], label='current', color='blue', linewidth=1.5)
ax1.plot(df['t_ms'], df['pwd_out'], label='pwd_out', color='red', linestyle=':', alpha=0.7)

ax1.set_title('PID регулятор (Setpoint vs current)', fontsize=14)
ax1.set_ylabel('Значення', fontsize=12)
ax1.legend(loc='upper right')
ax1.grid(True, linestyle=':', alpha=0.6)

# --- НИЖНІЙ ГРАФІК: Duty (ШІМ / Потужність) ---
ax2.plot(df['t_ms'], df['pwd_out'], label='pwd_out', color='green', linewidth=1.5)

ax2.set_title('Сигнал керування та Помилка', fontsize=14)
ax2.set_xlabel('t_ms', fontsize=12)
ax2.set_ylabel('Потужність / Помилка', fontsize=12)
ax2.legend(loc='upper right')
ax2.grid(True, linestyle=':', alpha=0.6)

plt.tight_layout()
plt.show()
