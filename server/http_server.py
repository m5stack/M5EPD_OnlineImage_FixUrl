from flask import Flask, request
from flask_cors import *
from werkzeug.utils import secure_filename
import os
from PIL import Image
import json
import time
import atexit
from apscheduler.schedulers.background import BackgroundScheduler
import shutil

devices = {}
app = Flask(__name__)

@app.route('/register',methods=['POST'])
@cross_origin()
def registerDevice():
    mac = request.form['mac']
    print('%s registed.' %(mac))

    if not mac.isalnum():
        return "{'info':'Invalid mac address'}", 406

    path = './%s' %mac

    if os.path.exists(path):
        devices[mac] = time.time()
        return "{'info':'Updated'}", 200

    os.mkdir(path)
    devices[mac] = time.time()

    return "{'info':'OK'}", 200

@app.route('/image/<string:mac>/<string:name>',methods=['GET'])
@cross_origin()
def getImage(mac, name):
    path = '%s/%s' %(mac, name)
    print(path)
    try:
        f = open(path, 'rb')
    except:
        return "{'info':'Image %s does not exist'}" %path, 404
    ret = f.read()
    f.close()
    return ret, 200

@app.route('/image/<string:mac>',methods=['GET'])
@cross_origin()
def getImages(mac):
    path = './%s' %(mac)
    print(path)
    if not os.path.exists(path):
        return "{'info':'MAC does not exist'}", 404
        
    images = []
    for root, dirs, files in os.walk(path):
        for f in files:
            images.append(f)

    print(devices)

    return json.dumps({'images':images}), 200

@app.route('/upload', methods=['POST'])
@cross_origin()
def uploadImage():
    mac = request.form['mac']
    file = request.files['file']
    path = './%s' %mac
    print(mac)
    if not os.path.exists(path):
        return "{'info':'MAC does not exist'}", 404

    try:
        Image.open(file.stream).verify()
    except:
        return "{'info':'Invalid image file'}", 406

    im = Image.open(file.stream)
    rgb_im = im.convert('RGB')
    jpgpath = path + '/' + str(secure_filename(file.filename)).rsplit(".", 1)[0] + ".jpg"
    try:
        print('saving ' + jpgpath)
        rgb_im.save(jpgpath)
    except:
        return "{'info':'Saving image error'}", 500
    
    devices[mac] = time.time()

    return "{'info':'OK'}", 201

def checkDevices():
    print(time.strftime("%A, %d. %B %Y %I:%M:%S %p"))
    keys = list(devices.keys())
    t = time.time() - 3600
    for k in keys:
        if devices[k] < t:
            print('device ' + k + ' timeout.')
            del devices[k]
            shutil.rmtree('./' + k)

# scheduler = BackgroundScheduler()
# scheduler.add_job(func=checkDevices, trigger="interval", seconds=60)
# scheduler.start()
# atexit.register(lambda: scheduler.shutdown())

if __name__ == '__main__':
    for root, dirs, files in os.walk('./'):
        for d in dirs:
            devices[d] = time.time()
    app.run('0.0.0.0', 80)
