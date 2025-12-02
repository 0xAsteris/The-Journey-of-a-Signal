import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# system parameters
m = 1.0           # mass (kg)
g = 9.81          # g (m/s^2)
z_p = 2.0         # position of charged plane
h = 1.0           # target height (m)
dt = 0.01         # time step (s)
T = 20            # total simulation time (s)
steps = int(T/dt)

# PID gains
Kp = 150
Ki = 60
Kd = 40

# initial conditions
z = 0           # starting position
v = 0           # starting velocity

z_init = z        
v_init = v

# control signal A (plane charge)
A = 0.0

# History for plotting
z_history = []
A_history = []
time = []

# PID state
integral = 0.0
prev_error = 0.0

for step in range(steps):
    t = step * dt
    error = h - z
    integral += error * dt
    derivative = (error - prev_error) / dt
    prev_error = error

    # apply PID
    A = Kp * error + Ki * integral + Kd * derivative
    A = max(A, 0.0)  # clamp A to be non-negative

    # compute forces on the ball
    if z < z_p:
        distance = max(z_p - z, 0.01)
        F_up = A / distance**2
    elif z >= z_p:
        z = z_p
        v = min(0.0, v)  # don't allow upward velocity through plane
        F_up = 0.0      # no upward force if ball overshoots the plane

    F_net = F_up - m * g
    a = F_net / m

    # differential changes in position & velocity
    v += a * dt
    z += v * dt

    # ground contact logic
    if z <= 0.0:
        z = 0.0
        v = max(0.0, v)  # prevent sticking

    # save data
    time.append(t)
    z_history.append(z)
    A_history.append(A)

# animate!

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(8, 7), gridspec_kw={'width_ratios': [3, 1]})

# set up the levitation plot
ax1.set_xlim(-0.5, 0.5)
ax1.set_ylim(0, z_p + 0.2)
ax1.set_xlabel('Position')
ax1.set_ylabel('Height (m)')
ax1.set_title('Electrical Levitation via PID Control')

ball, = ax1.plot([], [], 'o', color='blue', markersize=20)
plane = plt.Rectangle((-0.3, z_p), 0.6, 0.02, color='red')
ax1.add_patch(plane)
target_line = ax1.axhline(h, color='green', linestyle='--', label='Target height')
ax1.legend()

# set up the control signal bar plot
ax2.set_xlim(0, 1)
ax2.set_ylim(0, max(A_history) * 1.1)  # leave room above the max A
ax2.set_xticks([])
ax2.set_ylabel('Control Signal A')
ax2.set_title('Control Input')

bar = ax2.bar([0.5], [0], width=0.5, color='orange')[0]
text = ax2.text(0.5, 0, '', ha='center', va='bottom', fontsize=12)

def init():
    ball.set_data([], [])
    bar.set_height(0)
    text.set_text('')
    return ball, bar, text

def update(frame):
    z_val = z_history[frame]
    A_val = A_history[frame]

    # update ball position
    ball.set_data([0], [z_val])

    # update control signal bar height
    bar.set_height(A_val)
    text.set_text(f"A = {A_val:.1f}")
    text.set_y(A_val + 0.05 * ax2.get_ylim()[1])

    return ball, bar, text

ani = FuncAnimation(fig, update, frames=len(z_history), init_func=init, blit=True, interval=10)
plt.tight_layout()
plt.show()