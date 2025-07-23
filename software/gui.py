from tkinter import *
from tkinter import filedialog
import uuid
import time
import serial
from PIL import ImageTk, Image
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import os
import glob
import platform

# Initialize global variables
global msgs
global specific_device
    
specific_device = "/dev/ttyUSB_LAMP"  # Replace with your udev-assigned device name

msgs = []
ser = None  # Initialize ser to None to avoid NameError

def detect_serial_port():
    """Detect a specific serial port based on udev rules."""
    if os.path.exists(specific_device):
        return specific_device
    else:
        return "No specific serial port detected. Ensure the device is connected and udev rules are configured."

def append_message(msg=""):
    global info_text_field
    info_text_field.insert(END, msg + "\n")
    info_text_field.see(END)  # Scroll to the end

def set_pid_values():
    global p_entry, i_entry, d_entry, ser, internal_p_field, internal_i_field, internal_d_field
    p = p_entry.get()
    i = i_entry.get()
    d = d_entry.get()
    
    msg = f"$SP,{p},{i},{d}\n"
    try:
        ser.write(msg.encode())
    except:
        append_message("Device could not be found...")
        return
    internal_p_field.config(text=p)
    internal_i_field.config(text=i)
    internal_d_field.config(text=d)

def start_heating():
    global ttemp_entry, duration_entry, filename_entry
    global internal_ct_field, internal_sp_field, internal_remaining_field

    try:
        folderpath = filedialog.askdirectory()
    except:
        folderpath = os.path.expanduser("~")  # Default to home directory
        append_message(f"Defaulting to directory: {folderpath}")

    filename = filename_entry.get()

    if not filename:
        append_message("Choose a valid filename")
        return
    
    save_path = os.path.join(folderpath, filename + ".txt") 
    print(save_path)
    temps = ttemp_entry.get().split(',')
    durations = duration_entry.get().split(',')
    if len(temps) != len(durations):
        append_message("Each temperature value must have a corresponding duration") 
        return
    msg = f"$START,{len(temps)},"
    counter = 0
    for temp, dur in zip(temps, durations):
        if counter != len(temps) - 1:
            msg += f"{temp},{float(dur)*1000},"
        else: 
            msg += f"{temp},{float(dur)*1000}\n"
        counter += 1
    # Read received packets and append them to a file
    rcv = ""
    try:
        ser.read_all()
        ser.write(msg.encode())
    except:
        append_message("Device could not be found...")
    txts = []
    start_time = time.time()
    while "terminate" not in rcv:
        try:
            rcv = ser.readline().decode()
        except serial.SerialException:
            append_message("Serial connection lost. Please restart the device.")
            break
        if len(rcv.split(',')) == 4:
            print(rcv)
            rm_time = int((time.time() - start_time) / 60)
            internal_sp_field.config(text=rcv.split(',')[0].split(':')[1])
            internal_ct_field.config(text=rcv.split(',')[1].split(':')[1])
            internal_remaining_field.config(text=format(rm_time, ".2f"))
            txts.append(rcv)
            append_message(rcv)
            window.update()
    print("exit")
    with open(save_path, 'w') as file:
        file.writelines(txts)
        append_message(f"Measurements saved successfully -> {save_path}")

# Initialize the main window
window = Tk()
window.title("DXBiotech Lab LAMP")
window.geometry("1080x720")

# Create the info_text_field widget
info_text_field = Text(window, wrap='word')
info_text_field.grid(row=8, column=2, rowspan=3, columnspan=6)

config_label = Label(window, text="Configuration").grid(row=3, column=1, columnspan=5)

p_label = Label(window, text="Proportional")
p_entry = Entry(window)
p_label.grid(row=4, column=1)
p_entry.grid(row=4, column=2)
i_label = Label(window, text="Integral")
i_entry = Entry(window)
i_label.grid(row=5, column=1)
i_entry.grid(row=5, column=2)
d_label = Label(window, text="Derivative")
d_entry = Entry(window)
d_label.grid(row=6, column=1)
d_entry.grid(row=6, column=2)
set_button = Button(window, text="Set PID", command=set_pid_values).grid(row=7, column=1, columnspan=2, sticky="nswe")

ttemp_label = Label(window, text="Target Temperature").grid(row=4, column=4)
ttemp_entry = Entry(window)
ttemp_entry.grid(row=4, column=5)
duration_label = Label(window, text="Duration").grid(row=5, column=4)
duration_entry = Entry(window)
duration_entry.grid(row=5, column=5)
filename_label = Label(window, text="Filename").grid(row=6, column=4)
filename_entry = Entry(window)
filename_entry.grid(row=6, column=5)
start_button = Button(window, text="Start", command=start_heating).grid(row=7, column=4, columnspan=2, sticky="nswe")

internal_p_label = Label(window, text="Device P value:")
internal_i_label = Label(window, text="Device I value:")
internal_d_label = Label(window, text="Device D value:")
internal_sp_label = Label(window, text="Device Set Point:")
internal_ct_label = Label(window, text="Device Current Temp:")
internal_remaining_label = Label(window, text="Elapsed Time (min):")

internal_p_field = Label(window, text=msgs[0] if ser else "")
internal_i_field = Label(window, text=msgs[1] if ser else "")
internal_d_field = Label(window, text=msgs[2] if ser else "")
internal_sp_field = Label(window, text="")
internal_ct_field = Label(window, text=msgs[3] if ser else "")
internal_remaining_field = Label(window, text="")

internal_p_label.grid(row=4, column=7, sticky="e")
internal_p_field.grid(row=4, column=8)
internal_sp_label.grid(row=4, column=9, sticky="e")
internal_sp_field.grid(row=4, column=10)
internal_i_label.grid(row=5, column=7, sticky="e")
internal_i_field.grid(row=5, column=8)
internal_ct_label.grid(row=5, column=9, sticky="e")
internal_ct_field.grid(row=5, column=10)
internal_d_label.grid(row=6, column=7, sticky="e")
internal_d_field.grid(row=6, column=8)
internal_remaining_label.grid(row=6, column=9, sticky="e")
internal_remaining_field.grid(row=6, column=10)

# Detect serial port after initializing the GUI
serial_port_message = detect_serial_port()
append_message(serial_port_message)

if "No specific serial port detected" not in serial_port_message:
    try:
        ser = serial.Serial(serial_port_message, baudrate=9600, timeout=5)
        append_message(f"Connected to {serial_port_message}")
    except serial.SerialException:
        append_message("Failed to connect to the device. Please check the connection.")
        ser = None
else:
    ser = None

# Start the Tkinter main loop
window.mainloop()
