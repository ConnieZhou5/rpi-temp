const http = require('http');
const WebSocket = require('ws');

// Set up HTTP and WebSocket servers
const server = http.createServer();
const wss = new WebSocket.Server({ server });

// Broadcast function to send data to all connected clients
function broadcast(data) {
    console.log('Broadcasting data:', data); // Log broadcast data
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}

// Handle HTTP POST requests
server.on('request', (req, res) => {
    if (req.method === 'POST' && req.url === '/alert') {
        console.log('Received alert from accelerometer program'); // Log alert
        broadcast({ type: 'alert', message: 'Accelerometer threshold exceeded!' });
        res.end('Alert received');
    } else {
        res.statusCode = 404;
        res.end('Not found');
    }
});

// Handle WebSocket connections
wss.on('connection', (ws) => {
    console.log('New client connected');
    // broadcast({ type: 'alert', message: 'Test alert from server' });
});

// Start the server on port 3000
server.listen(3000, () => {
    console.log('Server running on http://localhost:3000');
});


// const WebSocket = require('ws');

// // Set up WebSocket server on port 3000
// const wss = new WebSocket.Server({ port: 3000 });

// // Initial GPS data (adjust to your needs)
// let gpsData = { lat: 40.0, lon: -75.0 };
// let alertCount = 0;

// wss.on('headers', (headers) => {
//   headers.push('Cross-Origin-Opener-Policy: same-origin');
//   headers.push('Cross-Origin-Embedder-Policy: require-corp');
// });

// // Broadcast function to send data to all connected clients
// function broadcast(data) {
//   wss.clients.forEach((client) => {
//     if (client.readyState === WebSocket.OPEN) {
//       client.send(JSON.stringify(data));
//     }
//   });
// }

// // Function to update GPS data slightly (for simulation)
// function updateGPSData() {
//   gpsData.lat += (Math.random() - 0.5) * 0.0001;
//   gpsData.lon += (Math.random() - 0.5) * 0.0001;
//   broadcast({ type: 'location', location: gpsData });
// }

// // Function to send periodic alerts
// function sendPeriodicAlert() {
//   alertCount++;
//   broadcast({ 
//     type: 'alert', 
//     message: `Test alert #${alertCount}: Periodic check at ${new Date().toLocaleTimeString()}`
//   });
// }

// // Handle WebSocket connections
// wss.on('connection', (ws) => {
//   console.log('New client connected');

//   // Send initial GPS data on connection
//   broadcast({ type: 'location', location: gpsData });

//   // Set up periodic updates when first client connects
//   if (wss.clients.size === 1) {
//     // Send GPS updates every 5 seconds
//     setInterval(updateGPSData, 5000);
    
//     // Send alerts every 10 seconds
//     setInterval(sendPeriodicAlert, 10000);
//   }

//   // Handle client disconnect
//   ws.on('close', () => {
//     console.log('Client disconnected');
//   });

//   // Handle any messages from client (optional)
//   ws.on('message', (message) => {
//     console.log('Received:', message.toString());
//   });
// });

// console.log('WebSocket server running on ws://172.20.10.9:3000');

