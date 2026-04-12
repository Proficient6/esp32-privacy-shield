/* ============================================
   CYBER SHIELD PRO — Frontend Application
   ESP32 Privacy Shield Configuration Dashboard
   ============================================ */

(() => {
  'use strict';

  // ===========================================
  // CONFIG & DEFAULTS
  // ===========================================
  const DEFAULTS = {
    distThresholdLock: 50,
    distThresholdAwake: 30,
    lockDelay: 3000,
    gestureNear: 5,
    gestureFar: 10,
    wavesRequired: 2
  };

  const WAVES_MIN = 1;
  const WAVES_MAX = 5;

  let statusPollTimer = null;
  let isConnected = false;

  // ===========================================
  // DOM REFERENCES
  // ===========================================
  const els = {
    // Sliders
    distThresholdLock:  document.getElementById('distThresholdLock'),
    distThresholdAwake: document.getElementById('distThresholdAwake'),
    lockDelay:          document.getElementById('lockDelay'),
    gestureNear:        document.getElementById('gestureNear'),
    gestureFar:         document.getElementById('gestureFar'),

    // Slider value displays
    lockDistValue:    document.getElementById('lockDistValue'),
    wakeDistValue:    document.getElementById('wakeDistValue'),
    lockDelayValue:   document.getElementById('lockDelayValue'),
    gestureNearValue: document.getElementById('gestureNearValue'),
    gestureFarValue:  document.getElementById('gestureFarValue'),

    // Stepper
    wavesValue: document.getElementById('wavesValue'),
    wavesDown:  document.getElementById('wavesDown'),
    wavesUp:    document.getElementById('wavesUp'),

    // Buttons
    btnSave:   document.getElementById('btnSave'),
    btnReset:  document.getElementById('btnReset'),
    btnExport: document.getElementById('btnExport'),

    // Status
    liveDistance:    document.getElementById('liveDistance'),
    liveState:      document.getElementById('liveState'),
    liveBle:        document.getElementById('liveBle'),
    connectionBadge: document.getElementById('connectionBadge'),
    connectionText:  document.getElementById('connectionText'),
    wifiIp:          document.getElementById('wifiIp'),

    // Toasts
    toastContainer: document.getElementById('toastContainer')
  };

  // ===========================================
  // SLIDER UTILITIES
  // ===========================================

  /** Format a value with unit for display */
  function formatValue(id, val) {
    const v = parseInt(val, 10);
    switch (id) {
      case 'distThresholdLock':
      case 'distThresholdAwake':
      case 'gestureNear':
      case 'gestureFar':
        return `${v} cm`;
      case 'lockDelay':
        return `${v} ms`;
      default:
        return String(v);
    }
  }

  /** Update the displayed value for a slider */
  function updateSliderDisplay(sliderId) {
    const slider = els[sliderId];
    if (!slider) return;
    const display = {
      distThresholdLock:  els.lockDistValue,
      distThresholdAwake: els.wakeDistValue,
      lockDelay:          els.lockDelayValue,
      gestureNear:        els.gestureNearValue,
      gestureFar:         els.gestureFarValue,
    }[sliderId];
    if (display) {
      display.textContent = formatValue(sliderId, slider.value);
    }
    // CSS: update fill gradient on WebKit browsers
    updateSliderFill(slider);
  }

  /** Apply a CSS gradient to visually fill the slider track */
  function updateSliderFill(slider) {
    const min = parseFloat(slider.min);
    const max = parseFloat(slider.max);
    const val = parseFloat(slider.value);
    const pct = ((val - min) / (max - min)) * 100;
    slider.style.background = `linear-gradient(90deg, #00dbe7 0%, #00f2ff ${pct}%, #333539 ${pct}%)`;
  }

  // ===========================================
  // STEPPER LOGIC
  // ===========================================
  let wavesRequired = DEFAULTS.wavesRequired;

  function renderWaves() {
    els.wavesValue.textContent = wavesRequired;
  }

  function stepWaves(delta) {
    wavesRequired = Math.max(WAVES_MIN, Math.min(WAVES_MAX, wavesRequired + delta));
    renderWaves();
  }

  // ===========================================
  // API CALLS
  // ===========================================

  /** Gather all current settings from the UI */
  function getSettingsFromUI() {
    return {
      distThresholdLock:  parseInt(els.distThresholdLock.value, 10),
      distThresholdAwake: parseInt(els.distThresholdAwake.value, 10),
      lockDelay:          parseInt(els.lockDelay.value, 10),
      gestureNear:        parseInt(els.gestureNear.value, 10),
      gestureFar:         parseInt(els.gestureFar.value, 10),
      wavesRequired:      wavesRequired
    };
  }

  /** Apply settings object to the UI */
  function applySettingsToUI(settings) {
    if (settings.distThresholdLock !== undefined) {
      els.distThresholdLock.value = settings.distThresholdLock;
      updateSliderDisplay('distThresholdLock');
    }
    if (settings.distThresholdAwake !== undefined) {
      els.distThresholdAwake.value = settings.distThresholdAwake;
      updateSliderDisplay('distThresholdAwake');
    }
    if (settings.lockDelay !== undefined) {
      els.lockDelay.value = settings.lockDelay;
      updateSliderDisplay('lockDelay');
    }
    if (settings.gestureNear !== undefined) {
      els.gestureNear.value = settings.gestureNear;
      updateSliderDisplay('gestureNear');
    }
    if (settings.gestureFar !== undefined) {
      els.gestureFar.value = settings.gestureFar;
      updateSliderDisplay('gestureFar');
    }
    if (settings.wavesRequired !== undefined) {
      wavesRequired = settings.wavesRequired;
      renderWaves();
    }
  }

  /** Load settings from ESP32 */
  async function loadSettings() {
    try {
      const resp = await fetch('/api/settings', { signal: AbortSignal.timeout(4000) });
      if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
      const data = await resp.json();
      applySettingsToUI(data);
      setConnected(true);
      showToast('Settings loaded from device', 'success');
    } catch (err) {
      console.warn('Could not load settings:', err.message);
      setConnected(false);
      showToast('Running in offline mode — using defaults', 'info');
    }
  }

  /** Save settings to ESP32 */
  async function saveSettings() {
    const settings = getSettingsFromUI();
    els.btnSave.classList.add('saving');
    els.btnSave.innerHTML = '<span class="spinner"></span> Saving...';

    try {
      const resp = await fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(settings),
        signal: AbortSignal.timeout(5000)
      });
      if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
      showToast('Settings saved to device ✓', 'success');
    } catch (err) {
      console.error('Save failed:', err.message);
      showToast('Failed to save — check connection', 'error');
    } finally {
      els.btnSave.classList.remove('saving');
      els.btnSave.innerHTML = `<svg viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg> Save to Device`;
    }
  }

  /** Reset all settings to defaults */
  function resetDefaults() {
    applySettingsToUI(DEFAULTS);
    showToast('Reset to factory defaults', 'info');
  }

  /** Export current settings as JSON file */
  function exportConfig() {
    const settings = getSettingsFromUI();
    const blob = new Blob([JSON.stringify(settings, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'privacy_shield_config.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    showToast('Configuration exported ↓', 'success');
  }

  // ===========================================
  // LIVE STATUS POLLING
  // ===========================================

  const STATE_NAMES = { 0: 'Present', 1: 'Away', 2: 'Locking' };
  const STATE_CLASSES = { 0: 'state-present', 1: 'state-away', 2: 'state-locking' };

  async function pollStatus() {
    try {
      const resp = await fetch('/api/status', { signal: AbortSignal.timeout(3000) });
      if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
      const data = await resp.json();

      // Distance
      const dist = data.distance !== undefined ? data.distance.toFixed(1) : '—';
      els.liveDistance.innerHTML = `${dist}<span class="status-unit">cm</span>`;

      // State
      const stateIdx = data.state !== undefined ? data.state : -1;
      const stateName = STATE_NAMES[stateIdx] || '—';
      els.liveState.textContent = stateName;
      els.liveState.className = 'status-value ' + (STATE_CLASSES[stateIdx] || '');

      // BLE
      const bleConnected = data.bleConnected;
      els.liveBle.textContent = bleConnected ? 'Paired' : 'Waiting';
      els.liveBle.className = 'status-value ' + (bleConnected ? 'state-present' : 'state-away');

      setConnected(true);
    } catch {
      setConnected(false);
    }
  }

  function startStatusPolling() {
    pollStatus();
    statusPollTimer = setInterval(pollStatus, 1500);
  }

  function stopStatusPolling() {
    if (statusPollTimer) clearInterval(statusPollTimer);
  }

  // ===========================================
  // CONNECTION STATE
  // ===========================================

  function setConnected(connected) {
    if (isConnected === connected) return;
    isConnected = connected;
    const badge = els.connectionBadge;
    if (connected) {
      badge.classList.remove('disconnected');
      badge.classList.add('connected');
      els.connectionText.textContent = 'Online';
    } else {
      badge.classList.remove('connected');
      badge.classList.add('disconnected');
      els.connectionText.textContent = 'Offline';
    }
  }

  // ===========================================
  // TOAST NOTIFICATIONS
  // ===========================================

  function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    
    const icons = {
      success: '✓',
      error: '✕',
      info: 'ℹ'
    };
    toast.innerHTML = `<strong>${icons[type] || ''}</strong> ${message}`;
    els.toastContainer.appendChild(toast);

    setTimeout(() => {
      toast.style.animation = 'toastOut 0.3s ease-in forwards';
      setTimeout(() => toast.remove(), 300);
    }, 3000);
  }

  // ===========================================
  // EVENT LISTENERS
  // ===========================================

  function initSliderListeners() {
    const sliderIds = [
      'distThresholdLock',
      'distThresholdAwake',
      'lockDelay',
      'gestureNear',
      'gestureFar'
    ];

    sliderIds.forEach(id => {
      const slider = els[id];
      if (!slider) return;
      // Update display on input
      slider.addEventListener('input', () => updateSliderDisplay(id));
      // Initialize fill
      updateSliderDisplay(id);
    });
  }

  function initStepperListeners() {
    els.wavesDown.addEventListener('click', () => stepWaves(-1));
    els.wavesUp.addEventListener('click', () => stepWaves(1));
    renderWaves();
  }

  function initButtonListeners() {
    els.btnSave.addEventListener('click', saveSettings);
    els.btnReset.addEventListener('click', resetDefaults);
    els.btnExport.addEventListener('click', exportConfig);
  }

  // ===========================================
  // KEYBOARD SHORTCUTS
  // ===========================================
  
  document.addEventListener('keydown', (e) => {
    // Ctrl/Cmd + S = Save
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
      e.preventDefault();
      saveSettings();
    }
  });

  // ===========================================
  // INIT
  // ===========================================

  function init() {
    initSliderListeners();
    initStepperListeners();
    initButtonListeners();

    // Try loading settings from ESP32
    loadSettings();

    // Start polling live status
    startStatusPolling();

    // Show IP if available (ESP32 sets this element server-side or we detect from URL)
    const currentHost = window.location.hostname;
    if (currentHost && currentHost !== '' && currentHost !== 'localhost' && currentHost !== '127.0.0.1') {
      els.wifiIp.textContent = currentHost;
    } else {
      els.wifiIp.textContent = 'Local Preview';
    }
  }

  // Boot
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();
