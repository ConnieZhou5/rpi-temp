# rpi-temp

wget https://github.com/vi/websocat/releases/latest/download/websocat.armhf


curl -X POST -H "Content-Type: application/json" \
-d '{"lat": 40.445, "lng": -79.945}' \
http://0.0.0.0:3000/location

