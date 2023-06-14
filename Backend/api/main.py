from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import pymysql
import time 
import datetime
import uvicorn

app = FastAPI()

# TODO: needs to include the origin of the flutter app
# when it is hosted on AWS.
origins = [
    # Copy the address that flutter is hosting the 
    # front end at and paste it here. Otherwise, flutter
    # cannot make the API call
    "https://datadrivenucsb.com",
    "http://localhost",
    "http://localhost:8000",
    "", # Arjun's IP
    "", # Brian's IP
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=['*'],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
async def root():
    return {"message": "Hello World from DataDriven API"}

@app.get("/dummy")
async def dummy():
    return {
        "engine_speed": 111,
        "vehicle_speed": 222,
        "runtime": 333,
        "odometer": 444,
        "lat": 34.416954 - round(time.time())%50*0.0001,
        "lng": -119.855367,
        "ang": 90
    }

@app.get("/dummy_multicar")
async def dummy_multicar():
    return [
        {
            "engine_speed": 111,
            "vehicle_speed": 222,
            "runtime": 333,
            "odometer": 444,
            "lat": round(34.416453 - round(time.time())%50*0.0001, 6),
            "lng": -119.855367,
            "ang": 90,
            "atTime": datetime.datetime.now().strftime("%H:%M:%S"),
            "atDate": datetime.date.today().strftime("%Y-%m-%d")
        },
        {
            "engine_speed": 444,
            "vehicle_speed": 333,
            "runtime": 222,
            "odometer": 111,
            "lat": round(34.415460 - round(time.time())%50*0.0001, 6),
            "lng": -119.857014,
            "ang": 90,
            "atTime": datetime.datetime.now().strftime("%H:%M:%S"),
            "atDate": datetime.date.today().strftime("%Y-%m-%d")
        }
    ]

@app.get("/cars")
async def get_cars():
    # Connect to AWS RDS
    conn = pymysql.connect(host = '<rds-link>', user = 'master', password = '', db = 'datadriven_db', charset = 'utf8')
    # Cursor
    cursor = conn.cursor(pymysql.cursors.DictCursor)
    sql = "USE datadriven_db"
    cursor.execute(sql)

    sql = "SHOW TABLES LIKE 'car%'"
    cursor.execute(sql)
    rows = cursor.fetchall()
    car_names = [row['Tables_in_datadriven_db (car%)'] for row in rows]
    return car_names

@app.get("/live_all_cars")
async def live_all_cars():
    # Connect to AWS RDS
    conn = pymysql.connect(host = '<rds-link>', user = 'master', password = '', db = 'datadriven_db', charset = 'utf8')

    cursor = conn.cursor(pymysql.cursors.DictCursor)
    sql = "USE datadriven_db"
    cursor.execute(sql)

    # Get all car ids
    # Car ids correspond to the car's data table names
    sql = "SHOW TABLES LIKE 'car_id_%'"
    cursor.execute(sql)
    rows = cursor.fetchall()
    car_ids = [row['Tables_in_datadriven_db (car_id_%)'] for row in rows]

    print("Valid car IDs")
    print(car_ids)

    # Iterate through each car data table and fetch latest data point
    ret = []
    for id in car_ids:
        sql = f"SELECT * FROM {id} ORDER BY id DESC LIMIT 1"
        print(sql)
        cursor.execute(sql)
        row = cursor.fetchone()
        print(row)

        # default to California coordinates (Northern/Western Hemisphere)
        # maintain API compatibility by calculating lat, lng
        lat = row["latitude"]
        lng = row["longitude"]

        del row["id"]
        del row["latitude"]
        del row["longitude"]

        row["car_id"] = id
        at_date = datetime.datetime.strptime(str(row["atDate"]), "%Y-%m-%d")
        row["atDate"] = at_date.strftime("%Y-%m-%d")
        at_time = datetime.datetime.strptime(str(row["atTime"]), "%H:%M:%S")
        row["atTime"] = at_time.strftime("%H:%M:%S")
        
        row["lat"] = lat
        row["lng"] = lng

        print(row)
        ret.append(row)

    print(ret)
    conn.close()
    return ret


@app.get("/live/{car_id}")
async def live_car(car_id):
    # Connect to AWS RDS
    conn = pymysql.connect(host = '<rds-link>', user = 'master', password = '', db = 'datadriven_db', charset = 'utf8')
    # Cursor
    cursor = conn.cursor(pymysql.cursors.DictCursor)
    sql = "USE datadriven_db"
    cursor.execute(sql)

    sql = f"SELECT * FROM {car_id} ORDER BY id DESC LIMIT 1"
    # print(sql)
    cursor.execute(sql)
    row = cursor.fetchone()

    conn.close()
    # print(row)

    # default to California coordinates (Northern/Western Hemisphere)
    # maintain API compatibility by calculating lat, lng
    lat = row["latitude"] 
    lng = row["longitude"]

    del row["id"]
    del row["latitude"]
    del row["longitude"]
    del row["atDate"]
    del row["atTime"]


    row["lat"] = lat
    row["lng"] = lng

    return row


@app.get("/fetch_all/{car_id}")
async def read_all_car_data(car_id):
    # Connect to AWS RDS
    conn = pymysql.connect(host = '<rds-link>', user = 'master', password = '', db = 'datadriven_db', charset = 'utf8')
    # Cursor
    cursor = conn.cursor(pymysql.cursors.DictCursor)
    sql = "USE datadriven_db"
    cursor.execute(sql)

    sql = f"SELECT * FROM {car_id}"
    # print(sql)
    cursor.execute(sql)
    conn.close()

    rows = cursor.fetchall()
    # print(rows)

    # default to California coordinates (Northern/Western Hemisphere)
    # maintain API compatibility by calculating lat, 
    for row in rows:
        lat = row["latitude"]
        lng = row["longitude"]
        atTime = row["atTime"]
        atDate = row["atDate"]

        del row["id"]
        del row["latitude"]
        del row["longitude"]
        del row["atDate"]
        # del row["atTime"]

        at_date = datetime.datetime.strptime(str(atDate), "%Y-%m-%d")
        row["atDate"] = at_date.strftime("%Y-%m-%d")
        at_time = datetime.datetime.strptime(str(atTime), "%H:%M:%S")
        row["atTime"] = at_time.strftime("%H:%M:%S")
        row["lat"] = lat
        row["lng"] = lng
    return rows

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
