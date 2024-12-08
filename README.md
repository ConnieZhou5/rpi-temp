# rpi-temp

wget https://github.com/vi/websocat/releases/latest/download/websocat.armhf


curl -X POST -H "Content-Type: application/json" \
-d '{"lat": 40.445, "lng": -79.945}' \
http://0.0.0.0:3000/location



[Unit]
Description=WalkGuard WebSocket Server
After=network.target

[Service]
ExecStart=/usr/bin/node /home/walkguard/Desktop/Code/websocket-server/server.js
Restart=always
User=walkguard
Group=walkguard
Environment=PATH=/usr/bin:/usr/local/bin
WorkingDirectory=/home/walkguard/Desktop/Code/websocket-server

[Install]
WantedBy=multi-user.target
