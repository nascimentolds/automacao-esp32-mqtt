// =================== CONFIGURAÇÕES DO USUÁRIO ===================
// Insira aqui a URL e as credenciais do seu broker MQTT
const brokerUrl = 'wss://SEU_BROKER_URL:8884/mqtt'; // Ex: 'wss://abcdef123.s1.eu.hivemq.cloud:8884/mqtt'

const options = {
  clientId: 'WebApp_Controller_' + Math.random().toString(16).substr(2, 8),
  username: 'SEU_USUARIO_MQTT',
  password: 'SUA_SENHA_MQTT'
};
// ===============================================================

// --- Tópicos MQTT ---
const commandTopicAC1 = 'sala/ac1/comando';
const commandTopicAC2 = 'sala/ac2/comando';
const stateTopicAC1 = 'sala/ac1/estado';
const stateTopicAC2 = 'sala/ac2/estado';

// --- Estado da Aplicação ---
let ac1State = { on: false, temp: 22, mode: 1, fan: 1, lastOnTime: 0 };
let ac2State = { on: false, temp: 22, mode: 1, fan: 1, lastOnTime: 0 };
let activeAC = null;
let uptimeInterval = null;

// Mapeamento de Modos
const modeMap = { 0: 'Auto', 1: 'Frio', 2: 'Calor', 3: 'Ventilar', 4: 'Desumidificar' };

// Elementos do DOM
const connectionStatusEl = document.getElementById('connection-status');
const allToggleButton = document.getElementById('all-toggle-button');
const swapButton = document.getElementById('swap-button');
const activeControlsSection = document.getElementById('active-controls-section');

// Conexão MQTT
const client = mqtt.connect(brokerUrl, options);

client.on('connect', () => {
  console.log('Conectado ao broker MQTT!');
  connectionStatusEl.classList.remove('status-disconnected');
  connectionStatusEl.classList.add('status-connected');
  connectionStatusEl.querySelector('.status-text').textContent = 'Conectado';
  client.subscribe([stateTopicAC1, stateTopicAC2]);
});

client.on('error', (err) => { console.error('Erro de conexão:', err); });
client.on('reconnect', () => { console.log('Reconectando...'); });

client.on('message', (topic, message) => {
  console.log(`Mensagem recebida no tópico ${topic}: ${message.toString()}`);
  try {
    const data = JSON.parse(message.toString());
    if (topic === stateTopicAC1) {
      Object.assign(ac1State, data);
    } else if (topic === stateTopicAC2) {
      Object.assign(ac2State, data);
    }
    updateUI();
  } catch (e) {
    console.error('Erro ao processar JSON:', e);
  }
});

function sendCommand(acNumber, payload) {
  const topic = (acNumber === 1) ? commandTopicAC1 : commandTopicAC2;
  client.publish(topic, JSON.stringify(payload), { qos: 1 }, (err) => {
    if (err) { console.error('Erro ao publicar comando:', err); }
    else { console.log(`Comando enviado para ${topic}:`, payload); }
  });
}

function updateUI() {
  updateCard('ac1', ac1State);
  updateCard('ac2', ac2State);
  activeAC = ac1State.on ? 1 : (ac2State.on ? 2 : null);
  if (ac1State.on || ac2State.on) {
    allToggleButton.textContent = 'Desligar Tudo';
  } else {
    allToggleButton.textContent = 'Ligar AC Padrão';
  }
  if (activeAC) {
    activeControlsSection.classList.add('visible');
    updateActiveControls();
  } else {
    activeControlsSection.classList.remove('visible');
  }
}

function updateCard(cardId, state) {
  const card = document.getElementById(`${cardId}-card`);
  const isActive = (cardId === 'ac1' && activeAC === 1) || (cardId === 'ac2' && activeAC === 2);

  card.className = `ac-card ${state.on ? 'on' : 'off'} ${isActive ? 'active' : ''}`;
  document.getElementById(`${cardId}-status`).querySelector('.status-text').textContent = state.on ? 'Ligado' : 'Desligado';
  document.getElementById(`${cardId}-temp`).textContent = state.temp;
  document.getElementById(`${cardId}-mode`).textContent = modeMap[state.mode] || 'Desconhecido';

  if (state.on) {
    startUptimeTimer(cardId, state.lastOnTime);
  } else {
    document.getElementById(`${cardId}-uptime`).textContent = '--';
  }
}

function updateActiveControls() {
  if (!activeAC) return;
  const state = (activeAC === 1) ? ac1State : ac2State;
  document.getElementById('active-temp').textContent = `${state.temp}°C`;
  document.querySelectorAll('.mode-button').forEach(btn => {
    btn.classList.toggle('active', parseInt(btn.dataset.mode) === state.mode);
  });
}

function startUptimeTimer(cardId, lastOnTimeMillis) {
    if (uptimeInterval) clearInterval(uptimeInterval);
    if(lastOnTimeMillis === 0) {
      document.getElementById(cardId + '-uptime').textContent = "Calculando...";
      return;
    }
    uptimeInterval = setInterval(() => {
        const elapsed = Date.now() - (performance.timeOrigin + lastOnTimeMillis);
        if (elapsed < 0) {
            document.getElementById(cardId + '-uptime').textContent = 'Aguardando...';
            return;
        }
        const hours = Math.floor(elapsed / 3600000);
        const minutes = Math.floor((elapsed % 3600000) / 60000);
        const seconds = Math.floor((elapsed % 60000) / 1000);
        document.getElementById(cardId + '-uptime').textContent = `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
    }, 1000);
}

swapButton.addEventListener('click', () => {
  if (ac1State.on) {
    ac1State.on = false;
    sendCommand(2, { on: true });
  } else if (ac2State.on) {
    ac2State.on = false;
    sendCommand(1, { on: true });
  } else {
    sendCommand(1, { on: true });
  }
  updateUI();
});

allToggleButton.addEventListener('click', () => {
  if (ac1State.on || ac2State.on) {
    ac1State.on = false;
    ac2State.on = false;
    sendCommand(1, { on: false });
    setTimeout(() => sendCommand(2, { on: false }), 200);
  } else {
    sendCommand(1, { on: true });
  }
  updateUI();
});

document.getElementById('temp-up').addEventListener('click', () => {
  if (!activeAC) return;
  let state = (activeAC === 1) ? ac1State : ac2State;
  if (state.temp < 30) { sendCommand(activeAC, { temp: state.temp + 1 }); }
});

document.getElementById('temp-down').addEventListener('click', () => {
  if (!activeAC) return;
  let state = (activeAC === 1) ? ac1State : ac2State;
  if (state.temp > 16) { sendCommand(activeAC, { temp: state.temp - 1 }); }
});

document.querySelectorAll('.mode-button').forEach(btn => {
  btn.addEventListener('click', () => {
    if (!activeAC) return;
    const newMode = parseInt(btn.dataset.mode);
    sendCommand(activeAC, { mode: newMode });
  });
});

// Inicializa a interface
updateUI();
