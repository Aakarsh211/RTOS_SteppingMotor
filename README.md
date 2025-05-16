# RTOS_SteppingMotor

A real-time embedded system project built on FreeRTOS and lwIP, running on the Zybo Z7-10 board. This system controls a stepper motor via a browser interface with support for real-time updates, LED-based feedback, and emergency stop handling.

Features:
- Web interface for real-time motor control using HTTP GET/POST.
- Adjustable speed, direction, and stepping mode (full, half, wave).
- Emergency stop with flashing LED indicator and task-safe shutdown.
- Multi-tasking architecture built with FreeRTOS.
- lwIP TCP/IP stack integration for HTTP server functionality.

Technologies Used:
- C
- FreeRTOS
- lwIP
- Zybo Z7-10 Board
- Xilinx SDK/Vitis
- TCP/IP Stack
- GPIO
- Interrupts
- Multithreading
- External Sensors

Setup:
1. Clone Repo.
2. Open in Vitis or Xilinx SDK.
3. Flash to Zybo Z7-10 board with proper bitstream.
4. Access the web UI via the device's IP address.
