import pymysql
import pandas as pd

# Connect to AWS RDS
conn = pymysql.connect(host = '<rds-link>', user = 'master', password = '', db = 'datadriven_db', charset = 'utf8')

# Cursor
cursor = conn.cursor(pymysql.cursors.DictCursor)

# Execute the following command
sql = "use datadriven_db"
cursor.execute(sql)

sql = "DROP TABLE IF EXISTS test_auto"
cursor.execute(sql)

sql = "CREATE TABLE test_auto(id INT PRIMARY KEY AUTO_INCREMENT, at_time TIME, latitude DECIMAL(8, 6), longitude DECIMAL(9, 6), speed INT)"
cursor.execute(sql)

sql = "INSERT INTO test_auto(at_time, latitude, longitude, speed) VALUES('20:18:00', 34.420560, -119.862550, 60)"
cursor.execute(sql)

# Commit
conn.commit()

# Close
conn.close()

#result = cursor.fetchall()
#result = pd.DataFrame(result)
#result

