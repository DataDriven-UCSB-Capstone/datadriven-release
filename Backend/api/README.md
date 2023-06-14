This is the API that serves data to the front-end by making SQL queries to the AWS RDS database in the backend.

See https://api.datadrivenucsb.com/docs for the API docs

To install the dependencies, run
```
pip install fastapi[all] pymysql
```

To run this server, run 
```
uvicorn main:app --host 0.0.0.0 --port 8000
```

To ssh into the EC2 instance hosting our server, run
```
ssh -i "<.pem>" <ec2-link>
```
