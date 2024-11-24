const http = require('http');
const express = require('express');
const socketIO = require('socket.io');
const app = express();
const server = http.createServer(app);
const io = socketIO(server);
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

server.listen(3000, function() {
  console.log('Server listening on port', 3000);
});

app.use(express.static(__dirname + '/public'));

// Configuración del puerto serial
const port = new SerialPort({
    path: 'COM5',
    baudRate: 115200,
});

// Parser para leer datos en líneas
const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

// Evento para cuando se abre el puerto
port.on('open', () => {
    console.log('Puerto serial abierto');
});

// Evento para recibir datos desde el ESP32
parser.on('data', function(data) {
    console.log(`Datos recibidos desde ESP32: ${data}`);

    // Expresiones regulares ajustadas para capturar las variables
    const humidityMatch = data.match(/Humedad:\s*([\d.]+)\s*%/);
    const temperatureMatch = data.match(/Temperatura:\s*([\d.]+)\s*\*C/);
    const entrada1Match = data.match(/Entrada 1\s*=\s*(\d+)/);
    const entrada2Match = data.match(/Entrada 2\s*=\s*(\d+)/);
    const salida1Match = data.match(/Salida 1\s*=\s*(\d+)/);
    const salida2Match = data.match(/Salida 2\s*=\s*(\d+)/);
    const horaMatch = data.match(/Hora\s*=\s*(\d{1,2}:\d{1,2}:\d{1,2})/);
    const keyPressedMatch = data.match(/Key pressed:\s*(\d+)/);
    
    const extractedData = {};

    // Asignación de valores extraídos, si están presentes
    if (humidityMatch) extractedData.humidity = humidityMatch[1];
    if (temperatureMatch) extractedData.temperature = temperatureMatch[1];
    if (entrada1Match) extractedData.entrada1 = entrada1Match[1];
    if (entrada2Match) extractedData.entrada2 = entrada2Match[1];
    if (salida1Match) extractedData.salida1 = salida1Match[1];
    if (salida2Match) extractedData.salida2 = salida2Match[1];
    if (horaMatch) extractedData.hora = horaMatch[1];
    if (keyPressedMatch) extractedData.keyPressed = keyPressedMatch[1];

    // Verifica que se hayan extraído datos
    if (Object.keys(extractedData).length > 0) {
        // Emisión de datos al frontend
        io.emit('data', extractedData);
    } else {
        // Si no hay coincidencias, se emite el log como un mensaje adicional
        io.emit('log', data);
    }
});

// Manejo de errores en el puerto serial
port.on('error', (err) => {
    console.error('Error en el puerto serial:', err.message);
});
