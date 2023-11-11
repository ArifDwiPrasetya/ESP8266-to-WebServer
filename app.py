import os
from os.path import join, dirname
from dotenv import load_dotenv

from flask import Flask, render_template, jsonify, request
from pymongo import MongoClient
from datetime import datetime
import pytz

dotenv_path = join(dirname(__file__), '.env')
load_dotenv(dotenv_path)

MONGODB_URI = os.environ.get("MONGODB_URI")
DB_NAME =  os.environ.get("DB_NAME")

client = MongoClient(MONGODB_URI)

db = client[DB_NAME]

app= Flask(__name__)

wita = pytz.timezone('Asia/Makassar')
# today = datetime.now(wita)

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/dataSensor', methods=['GET'])
def show_sensor():
    latestData = db.sensor.find_one({},{'_id':False},sort=[('$natural', -1)])
    return jsonify({'latestData' : latestData})

@app.route('/penyiraman', methods=['GET'])
def waktu_siram():
    latestSiram = db.waktu_penyiraman.find_one({},{'_id':False},sort=[('$natural', -1)])
    return jsonify({'waktu_penyiraman' : latestSiram})

@app.route('/status', methods=['GET'])
def status_siram():
    status_data = db.status_penyiraman.find_one({}, {'_id': False}, sort=[('$natural', -1)])
    status = status_data.get('status') if status_data else 'Status penyiraman False'
    return jsonify({'status':status})


@app.route('/status_perangkat', methods=['GET'])
def getstatus_perangkat():
    doc = {'status_perangkat':'Online'}
    db.status_perangkat.insert_one(doc)
    count = 0
    listStatusPerangkat = list(db.status_perangkat.find({}, {'_id': False}))
    for i in listStatusPerangkat: count+=1
    if count > 10 : 
        status_perangkat='Offline'
    else : status_perangkat='Online'

    return jsonify({ 'message' : status_perangkat})

@app.route('/status_perangkat', methods=['POST'])
def poststatus_perangkat():
    db.status_perangkat.delete_many({})
    return jsonify({'message':'sedang online'})

    

@app.route('/delete_status', methods=['GET'])
def delete_status():
    db.status_penyiraman.delete_many({})

    return jsonify({'message': 'status False, penyiraman telah dilakukan'})

@app.route('/penyiraman', methods=['POST'])
def penyiraman():
    today = datetime.now(wita)
    time = today.strftime('%Y-%m-%d-%H-%M-%S')
    status_receive = request.form.get('status_siram')
    
    doc_waktu = {
        'waktu_penyiraman' : time
    }
    db.waktu_penyiraman.insert_one(doc_waktu)

    doc_status = {
        'status' : status_receive
    }

    db.status_penyiraman.insert_one(doc_status)
    
    return jsonify({'status_siram' : 'Berhasil'})

@app.route('/dataSensor', methods=['POST'])
def save_sensor():
    suhu_receive = request.form.get('dataSuhu')
    kelembabanU_receive = request.form.get('dataKelembaban')
    kelembabanT_receive = request.form.get('dataTanah')

    today = datetime.now(wita)
    time = today.strftime('%Y-%m-%d-%H-%M-%S')

    doc = {
        'suhu' : suhu_receive,
        'kelembaban_udara' : kelembabanU_receive,
        'kelembaban_tanah' : kelembabanT_receive,
        'waktu' : time
        
    }
    db.sensor.insert_one(doc)
    return jsonify({'message' : 'Data Sensor DHT Berhasil Terkirim'})



if __name__== '__main__':
    app.run('0.0.0.0', port=5000, debug=True)

