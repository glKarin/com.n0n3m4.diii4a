class Server:
  def __init__(self):
    self.challenge = 0
    self.addr = ("0.0.0.0", 0)
    self.port = 25600
    self.prejoinactions = 0
    self.product = ""
    self.params = {}
    self.paramsallowed = "port;players;maxplayers;level;gametype;version"
    self.visible = False
    self.lastheartbeat = time.time()

  def join(self):
    self.challenge = random.randint(0x00000000, 0x7FFFFFFF)
    self.port = self.addr[1] - 1
    sendto("\1" + struct.pack("i", self.challenge), addr)

  def challengeresponse(self, data):
    params = re.findall(";(\w+);([^;]*)", data)
    valid = False
    for param in params:
      key = param[0]
      val = param[1]
      if(key == "challenge"):
        if(val == str(self.challenge)):
          valid = True
        continue
      if(key == "product"):
        self.product = val
        continue
      try:
        self.paramsallowed.split(";").index(key)
        self.params[key] = val
      except:
        pass
    if(not self.visible):
      if(valid):
        self.visible = True
    else:
      self.lastheartbeat = time.time()

  def statechanged(self, data):
    print("state changed: " + data)