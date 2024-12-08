const http = require('http');
const admin = require('firebase-admin');

// Initialize Firebase Admin SDK
const serviceAccount = require('websocket-server/walkguard-88-firebase-adminsdk-gyt8l-332f029a2f.json'); 
admin.initializeApp({
    credential: admin.credential.cert(serviceAccount),
});

const db = admin.firestore();

// Set up HTTP server
const server = http.createServer();

// Function to save location data to Firestore
async function saveLocation(locationData) {
    try {
        await db.collection('locations').add({
            ...locationData,
            timestamp: new Date(),
        });
        console.log('Location data saved to Firestore');
    } catch (error) {
        console.error('Error saving location data:', error);
    }
}

// Function to save alerts to Firestore
async function saveAlert(alertMessage) {
    try {
        await db.collection('alerts').add({
            message: alertMessage,
            timestamp: new Date(),
        });
        console.log('Alert saved to Firestore');
    } catch (error) {
        console.error('Error saving alert:', error);
    }
}

// Handle HTTP POST requests for GPS data and alerts
server.on('request', (req, res) => {
    console.log(`[${new Date().toISOString()}] ${req.method} request from ${req.socket.remoteAddress} to ${req.url}`);

    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    if (req.method === 'POST' && req.url === '/location') {
        let body = '';
        req.on('data', (chunk) => {
            body += chunk;
        });
        req.on('end', () => {
            try {
                const locationData = JSON.parse(body);
                console.log('Received GPS location data:', locationData);
                saveLocation(locationData); // Save location data to Firestore
                res.end('Location received');
            } catch (error) {
                console.error('Error parsing JSON:', error);
                res.statusCode = 400;
                res.end('Invalid JSON');
            }
        });
    } else if (req.method === 'POST' && req.url === '/alert') {
        console.log('Received alert from accelerometer program');
        saveAlert('Accelerometer threshold exceeded!'); // Save alert to Firestore
        res.end('Alert received');
    } else {
        res.statusCode = 404;
        res.end('Not found');
    }
});

// Start the server on port 3000
server.listen(3000, '0.0.0.0', () => {
    console.log('Server running on http://0.0.0.0:3000');
});
