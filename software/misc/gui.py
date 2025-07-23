
from tkinter import *
from tkinter import filedialog
import uuid
import time
import serial
from PIL import ImageTk,Image
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import os


global msgs

def append_message(msg = ""):
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
    internal_p_field.config(text=p);internal_i_field.config(text=i);internal_d_field.config(text=d)

def start_heating():
    global ttemp_entry, duration_entry, filename_entry
    global internal_ct_field, internal_sp_field, internal_remaining_field

    folderpath = filedialog.askdirectory()
    filename = filename_entry.get()

    if not filename:
        append_message("choose valid filename")
        return
    
    save_path = os.path.join(folderpath, filename+".txt") 
    print(save_path)
    temps = ttemp_entry.get().split(',')
    durations = duration_entry.get().split(',')
    if len(temps) != len(durations):
        append_message("each temperature value must have corresponding duration") 
        return
    msg = f"$START,{len(temps)},"
    counter = 0
    for temp,dur in zip(temps,durations):
        if counter != len(temps) -1:
            msg+=f"{temp},{float(dur)*1000},"
        else: 
            msg+=f"{temp},{float(dur)*1000}\n"
        counter += 1
    # here read received packets and append them to a file
    rcv = ""
    try:
        ser.read_all()
        ser.write(msg.encode())
    except:
        append_message("Device could not be found...")
    txts = []
    start_time = time.time()
    while "terminate" not in rcv:
        rcv = ser.readline().decode()
        if(len(rcv.split(',')) == 4):
            print(rcv)
            rm_time = int((time.time() - start_time) / 60)
            internal_sp_field.config(text=rcv.split(',')[0].split(':')[1])
            internal_ct_field.config(text=rcv.split(',')[1].split(':')[1])
            internal_remaining_field.config(text = format(rm_time,".2f"))
            txts.append(rcv)
            append_message(rcv)
            window.update()
    print("exit")
    with open(save_path, 'w') as file:
        file.writelines(txts)
        append_message(f"measurements saved succesfully -> {save_path}")
        
try:
    ser = serial.Serial("/dev/tty.usbserial-11120", baudrate=9600, timeout=5)
    if not ser.is_open:
        ser.open()
    rcv = ""
    while "ready" not in rcv:
        rcv = ser.readline().decode()
        if len(rcv) != 0:
            print(rcv)
            msgs = rcv.split(',')
            print(len(msgs))
            break
except:
    append_message("Device could not be found...")

window = Tk()
window.title("DXBiotech Lab LAMP")
window.geometry("1080x720")


info_text_field = Text(window, wrap='word')
info_text_field.grid(row=8, column= 2, rowspan=3, columnspan=6)

config_label = Label(window, text="Configuration").grid(row=3,column=1, columnspan=5)

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
set_button = Button(window, text="Set PID", command=set_pid_values).grid(row=7, column=1, columnspan=2,sticky="nswe")


ttemp_label = Label(window, text="Target Temperature").grid(row=4, column=4)
ttemp_entry=Entry(window)
ttemp_entry.grid(row=4, column=5)
duration_label = Label(window, text="Duration").grid(row=5, column=4)
duration_entry=Entry(window)
duration_entry.grid(row=5, column=5)
filename_label = Label(window, text="filename").grid(row=6, column=4)
filename_entry = Entry(window)
filename_entry.grid(row=6, column=5)
start_button = Button(window, text="Start", command=start_heating).grid(row=7, column=4, columnspan=2, sticky="nswe")


internal_p_label = Label(window, text = "Device P value :" )
internal_i_label = Label(window, text = "Device I value :" )
internal_d_label = Label(window, text = "Device D value :" )
internal_sp_label = Label(window, text = "Device Set Point :" )
internal_ct_label = Label(window, text = "Device Current Temp :" )
internal_remaining_label = Label(window, text = "Elapsed Time (min) :")

internal_p_field = Label(window, text=msgs[0])
internal_i_field = Label(window, text=msgs[1])
internal_d_field = Label(window, text=msgs[2])
internal_sp_field = Label(window, text="")
internal_ct_field = Label(window, text=msgs[3])
internal_remaining_field = Label(window, text="")

internal_p_label.grid(row=4, column=7, sticky="e");internal_p_field.grid(row=4, column=8);internal_sp_label.grid(row=4, column=9, sticky="e");internal_sp_field.grid(row=4, column=10)
internal_i_label.grid(row=5, column=7, sticky="e");internal_i_field.grid(row=5, column=8);internal_ct_label.grid(row=5, column=9, sticky="e");internal_ct_field.grid(row=5, column=10)
internal_d_label.grid(row=6, column=7, sticky="e");internal_d_field.grid(row=6, column=8);internal_remaining_label.grid(row=6, column=9, sticky="e");internal_remaining_field.grid(row=6, column=10)

# # Create your Matplotlib plot (replace with your code)
# monitor_label = Label(window, text="Temperature Monitor")
# monitor_label.grid(row=8, column=1, columnspan=7)

# x = [i for i in range(10)]
# y = [0 for i in range(10)]  # Replace with meaningful temperature values

# # Create the figure with a reduced size for a more compact plot
# fig, ax = plt.subplots(figsize=(4, 2))  # Adjust figure size as needed
# # Generate the bar plot
# ax.plot(x, y, color='skyblue')  # Consider customizing color

# # Set clear and concise labels with smaller font sizes
# ax.set_xlabel('Time (s)', fontsize=5)
# ax.set_ylabel('Temperature (°C)', fontsize=5)  # Use degree symbol for clarity

# # Adjust tick labels (optional)
# ax.tick_params(axis='both', which='major', labelsize=3)  # Adjust tick label size

# # Customize grid lines (optional)
# ax.grid(True, linestyle='--', linewidth=0.5, color='gray', which='both', alpha=0.6)  # Adjust opacity

# # Tight layout to prevent overlapping elements
# plt.tight_layout()
# plot_canvas = FigureCanvasTkAgg(fig, master=window)
# plot_canvas.get_tk_widget().grid(row=9, column=1, rowspan=4, columnspan=10)



window.mainloop()
