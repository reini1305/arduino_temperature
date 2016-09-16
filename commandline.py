# -*- coding: utf-8 -*-
"""
Created on Thu May 14 15:04:31 2015

@author: reini
"""

import serial
import time
import sys
import getopt
import urllib2
import json
import numpy as np
from datetime import datetime
#import datetime
#import Skype4Py

def main(argv):  
    #default values
    serialport = '/dev/cu.usbmodemfd131'  
    delay = 60
    outfilename = 'temp.csv'  
    #withskype = False           
    try:                                
        opts, args = getopt.getopt(argv, "p:d:o:s", ["port=", "delay=", "output=", "--skype"])
    except getopt.GetoptError:
        sys.exit(2)
    for opt,arg in opts:
        if(opt in ("-p", "--port" )):
            serialport = arg
        if opt in ("-d", "--delay"):
            delay = float(arg)
        if opt in ("-o", "--output"):
            outfilename = arg
        #if opt in ("-s", "--skype"):
            #withskype = True
                             
    #if withskype:
        #skype = Skype4Py.Skype()
        #skype.FriendlyName = "Temperature Status"
        #skype.Attach()
        
    ser = serial.Serial(port=serialport,baudrate=9600,timeout=10)

    # ignore the first two newlines
    print ser.readline()
    print ser.readline()
    
    skip_pushing = 30

    while True:
        ser.write('m')
        temp = ser.readline()
        print temp
        #if withskype:
            #skype.currentUser.setMoodText("Current Office " + temp)
        
	# parse the temperature
        if(len(temp.split())<5):
            continue
        temperature = (temp.split()[2])
        std = (temp.split()[4])
        try:
            if(float(std)>1 ):
                continue
        except:
            continue
        if(np.isnan(float(std))):
            continue
        ts = int(time.time())
        #st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
        # append to csv file
        f = open(outfilename, 'a')
        f.writelines(str(ts)+";"+temperature + ";" + std +"\n")
        f.close()
        
        skip_pushing = skip_pushing+1
        
        if(skip_pushing>30):
          now = datetime.now()
          output_string = temperature+"°C "+str(now.hour)+":"+str(now.minute)+", "+str(now.day)+"."+str(now.month)+"."
          opener = urllib2.build_opener(urllib2.HTTPSHandler)
          request = urllib2.Request('https://timeline-api.getpebble.com/v1/user/glance', 
                                data=json.dumps({"slices":[{"layout":{"icon": "system://images/TIMELINE_SUN",
                                "subtitleTemplateString": output_string}}]}))
          request.add_header('Content-Type', 'application/json')
          request.add_header('X-User-Token', 'INSERT_HERE')
          request.get_method = lambda: 'PUT'
          url = opener.open(request)
          print url.msg
          skip_pushing=0
          
        time.sleep(delay)
    ser.close()


if __name__ == "__main__":
    main(sys.argv[1:])
