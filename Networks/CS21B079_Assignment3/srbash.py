import subprocess
import time

# List of packet loss percentages to test
loss_percentages = [0.1, 0.5, 1, 1.5, 2, 5]
# loss_percentages = [0.1]
# List of mean latency values to test (in milliseconds)
mean_latencies = [50, 100, 150, 200, 250, 500]
# mean_latencies = [50]

# Function to execute client.py and capture output
def run_client_and_capture_output():
    output_matrix = []
    for loss_percentage in loss_percentages:
        row = []
        for mean_latency in mean_latencies:
            # Generate command to run client.py with arguments
            scmd = ["python3", "srsend.py" ,"loco.jpg", "25587", "5", "0.5"]
            server = subprocess.Popen(scmd)
            time.sleep(0.1)

            cmd = ["sudo", "tc" ,"qdisc" ,"change" ,"dev", "lo", "root", "netem" ,"loss" ,f"{loss_percentage}%" ,"delay" ,f"{mean_latency}ms" ,"10ms" ,"distribution" ,"normal", "rate" ,"400KBps"]
            subprocess.run(cmd)
            cmd = ["python3", "srrec.py" ,"recc.jpg"]

            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode == 0:  # Check if subprocess ran successfully
                output = result.stdout.strip()
                print(output)
                row.append(output)
            else:
                print(f"Error running client.py with loss {loss_percentage}% and mean latency {mean_latency}ms.")
                print("Error details:", result.stderr)  # Print stderr for debugging
                row.append("Error")
            server.kill()
        output_matrix.append(row)
    subprocess.run(["sudo", "tc", "qdisc", "del" ,"dev" ,"lo" ,"root"])
    return output_matrix

# Main function
def main():
    
    output_matrix = run_client_and_capture_output()
    
    for row in output_matrix:
        print(row)

if __name__ == "__main__":
    main()
print("...............................")