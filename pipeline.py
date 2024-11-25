import sys
# Adjust sys.path for different directories
sys.path.append("/Users/sophiahuang/Desktop/18500/Code/AudioHat/WM8960_Audio_HAT_Code/")
sys.path.append("/Users/sophiahuang/Desktop/18500/Code/adxl345spi")
sys.path.append("/Users/sophiahuang/Desktop/18500/Code/gps")
sys.path.append("/Users/sophiahuang/Desktop/18500/Code/node_modules")
sys.path.append("/Users/sophiahuang/Desktop/18500/Code/websocket-server.js")

import time
from kld7 import KLD7
from playwav import play_audio
# from adxl345spi import ADXL345SPI  # Removed invalid import
# from gps import gps_reader  # Removed invalid import
import subprocess
import threading  # Added for threading

# Start WebSocket server
subprocess.Popen(["node", "websocket-server.js"])

def run_radar():
    try:
        with KLD7("/dev/ttyAMA2") as radar:
            # Set radar parameters
            radar.params.MIAN = -45
            radar.params.MAAN = 45
            radar.params.RRAI = 1
            radar.params.RSPI = 2
            radar.params.MIRA = 10
            radar.params.MARA = 55
                
            dist_dire = ""
                
            for target_info in radar.stream_TDAT():
                if target_info is not None:
                    if (0.5 <= target_info.distance <= 1.5 and -45 <= target_info.angle <= -15):
                        print("Target left 1m")
                        if dist_dire != "1_left":
                            dist_dire = "1_left"
                            play_audio(1, 'left')
                            sys.stdout.flush()
                    if (0.5 <= target_info.distance <= 1.5 and -15 <= target_info.angle <= 15):
                        print("Target mid 1m")
                        if dist_dire != "1_mid":
                            dist_dire = "1_mid"
                            play_audio(1, 'mid')
                            sys.stdout.flush()
                    if (0.5 <= target_info.distance <= 1.5 and 15 <= target_info.angle <= 45):
                        print("Target right 1m")
                        if dist_dire != "1_right":
                            dist_dire = "1_right"
                            play_audio(1, 'right')
                            sys.stdout.flush()
                    if (2.5 <= target_info.distance <= 3.5 and -45 <= target_info.angle <= -15):
                        print("Target left 3m")
                        if dist_dire != "3_left":
                            dist_dire = "3_left"
                            play_audio(3, 'left')
                    if (2.5 <= target_info.distance <= 3.5 and -15 <= target_info.angle <= 15):
                        print("Target mid 3m")
                        if dist_dire != "3_mid":
                            dist_dire = "3_mid"
                            play_audio(3, 'mid')
                    if (2.5 <= target_info.distance <= 3.5 and 15 <= target_info.angle <= 45):
                        print("Target right 3m")
                        if dist_dire != "3_right":
                            dist_dire = "3_right"
                            play_audio(3, 'right')
                    if (4.5 <= target_info.distance <= 5.5 and -45 <= target_info.angle <= -15):
                        print("Target left 5m")
                        if dist_dire != "5_left":
                            dist_dire = "5_left"
                            play_audio(5, 'left')
                    if (4.5 <= target_info.distance <= 5.5 and -15 <= target_info.angle <= 15):
                        print("Target mid 5m")
                        if dist_dire != "5_mid":
                            dist_dire = "5_mid"
                            play_audio(5, 'mid')
                    if (4.5 <= target_info.distance <= 5.5 and 15 <= target_info.angle <= 45):
                        print("Target right 5m")
                        if dist_dire != "5_right":
                            dist_dire = "5_right"
                            play_audio(5, 'right')
                    
            time.sleep(1)

    except KeyboardInterrupt:
        print("Radar process interrupted by user")
        if radar._port.is_open:
            radar.close()
            print("Serial connection closed")

def run_gps_fall_detection():
    # Run rungps.py as a subprocess
    subprocess.Popen(["python", "gps/rungps.py"])
    # Run the compiled adxl345spi binary
    subprocess.Popen(["sudo", "./adxl345spi/adxl345spi"])  # Ensure adxl345spi.c is compiled

def main():
    try:
        # Create threads for each subsystem
        radar_thread = threading.Thread(target=run_radar)
        gps_fall_thread = threading.Thread(target=run_gps_fall_detection)
        
        # Start threads
        radar_thread.start()
        gps_fall_thread.start()
        
        # Join threads
        radar_thread.join()
        gps_fall_thread.join()

        # TODO: websocket

    except KeyboardInterrupt:
        print("Process interrupted by user")
        # ...existing cleanup code...
        # terminate subprocesses if needed

if __name__ == "__main__":
    main()
