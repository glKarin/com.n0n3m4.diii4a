#!/usr/bin/python -u

# GameAgent
import socket
import random
import struct
import time
import re

# global user/server arrays
servers = {}
players = {}

# load user thread
execfile("user.py")
execfile("server.py")

# listen on socket
sockListen = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sockListen.bind(("0.0.0.0", 9005))
print("Listening on UDP port 9005.")

def sendto(data, addr):
  sockListen.sendto(data, addr)

# format address
def fmta(addr):
  return addr[0] + ":" + str(addr[1])

def server_join(data, addr):
  if(servers.has_key(fmta(addr))):
    print("Join request from server already joined!")
  print("Server join from " + fmta(addr))
  s = Server()
  s.addr = addr
  s.join()
  servers[fmta(addr)] = s

def server_challengeresponse(data, addr):
  if(servers.has_key(fmta(addr))):
    servers[fmta(addr)].challengeresponse(data)
  else:
    # server didn't join yet, make server join
    server_join(data, addr)

def server_statechanged(data, addr):
  if(servers.has_key(fmta(addr))):
    if(servers[fmta(addr)].visible):
      servers[fmta(addr)].statechanged(data)
    else:
      if(servers[fmta(addr)].prejoinactions > 5):
        servers.pop(fmta(addr))
        print("Server sent too many pre join actions, removing!")
      else:
        servers[fmta(addr)].prejoinactions += 1
  else:
    print("State change from server who didn't join yet!")
    # server didn't join yet, make server join
    server_join(data, addr)

def enum_trigger(data, addr):
  p = Player()
  p.addr = addr
  
  buffer = "s"
  topop = ""
  for key in servers:
    server = servers[key]
    if(time.time() - server.lastheartbeat > 300):
      # 5m have passed since last heartbeat
      # server sends a heartbeat every 2.5 minutes
      # we'll kick the server out because he's not doing this
      print("Server " + fmta(server.addr) + " hasn't given a heartbeat since " + time.strftime("%c", time.gmtime(server.lastheartbeat)) + ", kicking!")
      topop += key + ","
      continue
    if(not server.visible):
      continue
    if(len(buffer) + 6 > 1024):
      sendto(buffer, addr)
      buffer = "s"
    parse = server.addr[0].split(".")
    buffer += struct.pack("BBBBH", int(parse[0]), int(parse[1]), int(parse[2]), int(parse[3]), server.port)
  sendto(buffer, addr)
  for key in topop.split(","):
    if(servers.has_key(key)):
      servers.pop(key)

# masterserver main loop
while True:
  data, addr = sockListen.recvfrom(1024)

  packets = {
    'q': server_join,
    '0': server_challengeresponse,
    'u': server_statechanged,
    
    'e': enum_trigger,
  }

  packetID = data[0]
  if(packets.has_key(packetID)):
    packets[packetID](data[1:], addr)
  else:
    print("Packet from " + addr[0] + " denied because of unknown packet " + str(ord(packetID)))

# server died
print("Server shutting down.")
