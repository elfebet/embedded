import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv('/Users/anton/Desktop/csv.txt')

fig, ax1 = plt.subplots(figsize=(10, 8))
ax1.plot(df['t_ms'], df['setpoint'], label='Setpoint (rpm)', color='orange', linestyle='--', linewidth=2)
ax1.plot(df['t_ms'], df['current'], label='Current (rpm)', color='blue', linewidth=1.5)
ax1.plot(df['t_ms'], df['pwd_out'], label='PWD out (0..200)', color='green', alpha=1)

ax1.set_title('PID control', fontsize=14)
ax1.set_ylabel('Values', fontsize=12)
ax1.legend(loc='upper right')
ax1.grid(True, linestyle=':', alpha=0.6)

plt.tight_layout()
plt.show()
