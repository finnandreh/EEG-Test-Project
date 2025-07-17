import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import re
from collections import deque

# ---- Config ----
PORT = 'COM7'              # Change this to your Arduino's COM port
BAUD = 115200
WINDOW = 60                # Plot last 60 seconds

# ---- Data Storage ----
time_data = deque(maxlen=WINDOW)
rms_data = deque(maxlen=WINDOW)
attention_data = deque(maxlen=WINDOW)
alpha_data = deque(maxlen=WINDOW)
theta_data = deque(maxlen=WINDOW)
delta_data = deque(maxlen=WINDOW)
beta_data = deque(maxlen=WINDOW)
state_data = deque(maxlen=WINDOW)

# ---- Setup Serial ----
ser = serial.Serial(PORT, BAUD, timeout=1)
print(f"Connected to {PORT} at {BAUD} baud")

# ---- Regex pattern for line parsing ----
pattern = re.compile(
    r"second:(\d+), RMS:([\d.]+), spikeCount:(\d+), alphaPower:([\d.]+), attention:([\d.]+), "
    r"thetaPower:([\d.]+), thetaIndex:([\d.]+), dominantThetaFreq:([\d.]+), "
    r"deltaPower:([\d.]+), deltaIndex:([\d.]+), dominantDeltaFreq:([\d.]+), "
    r"betaPower:([\d.]+), betaIndex:([\d.]+), dominantBetaFreq:([\d.]+), state:(.+)"
)

# ---- Plot Setup ----
fig, ax = plt.subplots(3, 1, figsize=(10, 8), sharex=True)

def update(frame):
    try:
        line = ser.readline().decode('utf-8').strip()
        match = pattern.match(line)
        if match:
            (
                second, rms, spikeCount, alpha, attention,
                theta, _, _, delta, _, _, beta, _, _, state
            ) = match.groups()

            time_data.append(int(second))
            rms_data.append(float(rms))
            attention_data.append(float(attention))
            alpha_data.append(float(alpha))
            theta_data.append(float(theta))
            delta_data.append(float(delta))
            beta_data.append(float(beta))
            state_data.append(state)

            # Clear plots
            for a in ax: a.cla()

            # RMS
            ax[0].plot(time_data, rms_data, label="RMS")
            ax[0].set_ylabel("RMS")
            ax[0].legend()
            ax[0].grid(True)

            # Attention
            ax[1].plot(time_data, attention_data, label="Attention", color="orange")
            ax[1].set_ylabel("Attention")
            ax[1].legend()
            ax[1].grid(True)

            # Band Powers
            ax[2].plot(time_data, alpha_data, label="Alpha")
            ax[2].plot(time_data, theta_data, label="Theta")
            ax[2].plot(time_data, delta_data, label="Delta")
            ax[2].plot(time_data, beta_data, label="Beta")
            ax[2].set_ylabel("Power")
            ax[2].set_xlabel("Time (s)")
            ax[2].legend()
            ax[2].grid(True)

            fig.suptitle(f"Brain State: {state_data[-1]}", fontsize=16)

    except Exception as e:
        print("Error:", e)

ani = FuncAnimation(fig, update, interval=200)
plt.tight_layout()
plt.show()
