import numpy as np
import matplotlib.pyplot as plt


data = np.array([[16.69288282, 27.39522314, 37.50861901, 48.86101273, 59.45915859, 109.22583022],
                 [16.59893977, 26.88038236, 37.21570254, 48.29719578, 58.92105059, 110.06191348],
                 [18.4272309, 28.05108506, 38.09395822, 49.21066562, 59.56897811, 110.02710474],
                 [17.03649836, 28.47229332, 37.51096635, 48.19042603, 60.08507816, 110.37837417],
                 [17.1356512, 30.21410083, 38.03739105, 49.79109815, 59.77025423, 110.94792633],
                 [22.35135763, 30.65020712, 42.73338917, 51.55404443, 61.47862378, 112.72451651]])

# Define packet loss percentages and latency means
packet_loss = [0.1, 0.5, 1, 1.5, 2, 5]
latency_means = [50, 100, 150, 200, 250, 500]

# Plot heatmap for the given data with the 'inferno' colormap
plt.figure(figsize=(10, 6))
heatmap = plt.imshow(data, cmap='Reds', interpolation='nearest')  # Using 'inferno' colormap for dark colors
plt.colorbar(label='Latency (ms)')
plt.title('Heatmap of Latency')
plt.xticks(np.arange(len(latency_means)), latency_means)
plt.yticks(np.arange(len(packet_loss)), packet_loss)
plt.xlabel('Latency (ms)')
plt.ylabel('Packet Loss (%)')

# Annotate the heatmap with the values of the matrix
for i in range(len(packet_loss)):
    for j in range(len(latency_means)):
        if j==5:
            plt.text(j, i, '{:.2f}'.format(data[i, j]),
                 ha='center', va='center', color='white')
        else :
            plt.text(j, i, '{:.2f}'.format(data[i, j]),
                 ha='center', va='center', color='black')

plt.show()