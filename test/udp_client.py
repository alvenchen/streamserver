import socket
import time
import _thread

UDP_IP = "10.68.68.158"
UDP_PORT = 443
MESSAGE = "Hello, World "

N = 10000

def sendUdp(threadName):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP

    print("start\n")
    for i in range(0, N):
        sock.sendto(bytes("{}{}\n".format(MESSAGE, i), "utf-8"), (UDP_IP, UDP_PORT))
        data, address = sock.recvfrom(4096)
        print("thread {} get:{}\n".format(threadName, data.decode('utf-8')))
        #time.sleep(0.001)

    print("done\n")

try:
   _thread.start_new_thread( sendUdp, ("Thread-1",) )
   _thread.start_new_thread( sendUdp, ("Thread-2",) )
   _thread.start_new_thread( sendUdp, ("Thread-3",) )
   _thread.start_new_thread( sendUdp, ("Thread-4",) )
except Exception as err:
   print("Error: unable to start thread: ", err)
 
while 1:
   pass