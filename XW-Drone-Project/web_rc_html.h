// web_rc_html.h
#ifndef WEB_RC_HTML_H
#define WEB_RC_HTML_H

const char webRCIndexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
  <meta name="theme-color" content="#14171a">
  <title>Controlador Dron XW</title>
  <style>
    :root {
      --bg: #14171a; --panel: #1c2126; --line: #2e363d; --text: #e6e9ec;
      --muted: #8b96a0; --amber: #f2a93b; --green: #4cc38a; --red: #e5484d;
      --stick-scale: 1.0; 
      --stick: calc(min(44dvh, 38vw) * var(--stick-scale)); 
      --edge: max(env(safe-area-inset-left), 3vw);
      --deadzone: 4%;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    html, body { margin: 0; height: 100%; overflow: hidden; overscroll-behavior: none; }
    
    body { 
      touch-action: none; user-select: none; -webkit-user-select: none; -webkit-touch-callout: none; 
      background: var(--bg); color: var(--text); font: 15px/1.2 system-ui, -apple-system, sans-serif; 
      transition: box-shadow 0.3s ease;
    }
    body.critical-battery { box-shadow: inset 0 0 40px 10px rgba(229, 72, 77, 0.6); animation: pulse-red 1s infinite alternate; }
    @keyframes pulse-red { from { box-shadow: inset 0 0 40px 10px rgba(229, 72, 77, 0.2); } to { box-shadow: inset 0 0 60px 20px rgba(229, 72, 77, 0.8); } }

    .app { display: grid; grid-template-rows: auto 1fr auto; height: 100dvh; padding: env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left); transition: all 0.3s ease; }
    
    .topbar { display: flex; align-items: center; justify-content: space-between; gap: 12px; padding: 8px var(--edge); transition: all 0.3s ease; z-index: 30;}
    .telemetry { flex: 1; display: flex; align-items: center; justify-content: center; flex-wrap: wrap; gap: 6px; min-width: 0; transition: opacity 0.3s ease;}
    .chip { padding: 5px 10px; border: 1px solid var(--line); border-radius: 6px; background: var(--panel); font-size: 13px; white-space: nowrap; font-variant-numeric: tabular-nums; }
    .chip[data-level="ok"] { color: var(--green); border-color: color-mix(in srgb, var(--green) 45%, var(--line)); }
    .chip[data-level="warn"] { color: var(--amber); border-color: color-mix(in srgb, var(--amber) 45%, var(--line)); }
    .chip[data-level="bad"] { color: var(--red); border-color: color-mix(in srgb, var(--red) 45%, var(--line)); }
    
    .hold-btn { position: relative; overflow: hidden; flex: none; width: clamp(104px, 15vw, 150px); height: 56px; display: flex; flex-direction: column; align-items: center; justify-content: center; border: 2px solid var(--btn-color); border-radius: 10px; background: var(--panel); color: var(--text); font: inherit; transition: all 0.3s ease;}
    .hold-btn--arm { --btn-color: var(--green); } .hold-btn--arm[data-state="armed"] { --btn-color: var(--amber); }
    .hold-btn--stop { --btn-color: var(--red); }
    .hold-btn__label { position: relative; z-index: 1; font-weight: 700; font-size: 16px; }
    .hold-btn__hint { position: relative; z-index: 1; font-size: 11px; color: var(--muted); }
    .hold-btn::before { content: ""; position: absolute; inset: 0; background: var(--btn-color); opacity: .35; transform: scaleX(0); transform-origin: left; transition: transform .15s ease-out; }
    .hold-btn.holding::before { transform: scaleX(1); transition: transform var(--hold-ms, 1000ms) linear; }
    .hold-btn.done { animation: flash .35s ease-out; }
    @keyframes flash { 50% { background: var(--btn-color); } }
    
    .menu { position: relative; z-index: 40; transition: all 0.3s ease;}
    .menu__toggle { width: 40px; height: 36px; display: grid; place-items: center; border: 1px solid var(--line); border-radius: 6px; background: var(--panel); color: var(--text); cursor: pointer;}
    .menu__panel { position: absolute; top: calc(100% + 6px); right: 0; z-index: 50; min-width: 240px; display: flex; flex-direction: column; padding: 6px; border: 1px solid var(--line); border-radius: 10px; background: var(--panel); box-shadow: 0 10px 30px rgba(0,0,0,.8); max-height: 80dvh; overflow-y: auto; }
    .menu__panel[hidden] { display: none; }
    .menu__item { padding: 12px 14px; border: 0; border-radius: 6px; background: none; color: var(--text); font: inherit; text-align: left; cursor: pointer; }
    .menu__item:active { background: var(--line); }
    .menu__item.active-mode { color: var(--amber); font-weight: bold; }
    
    .size-panel { border-top: 1px solid var(--line); padding: 8px; }
    .size-title { font-size: 12px; color: var(--muted); text-align: center; margin-bottom: 6px; }
    .size-grid { display: flex; gap: 4px; }
    .size-btn { flex: 1; padding: 8px 0; background: var(--bg); border: 1px solid var(--line); color: var(--text); border-radius: 4px; font-weight: bold; cursor: pointer;}
    .size-btn.active { background: var(--amber); color: #000; border-color: var(--amber); }

    .trim-panel { border-top: 1px solid var(--line); padding: 8px; }
    .trim-title { font-size: 12px; color: var(--muted); text-align: center; margin-bottom: 6px; }
    .trim-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 4px; }
    .trim-btn { padding: 8px 0; background: var(--bg); border: 1px solid var(--line); color: var(--text); border-radius: 4px; font-weight: bold; font-size: 16px; cursor: pointer;}
    .trim-btn:active { background: var(--line); }
    .trim-btn.reset { grid-column: 1 / -1; font-size: 12px; background: var(--red); color: #fff; border: none; padding: 6px; margin-top: 4px; }
    .trim-values { text-align: center; font-size: 12px; margin-top: 6px; color: var(--amber); font-variant-numeric: tabular-nums; }

    .stage { position: relative; min-height: 0; display: grid; place-items: center; overflow: hidden; z-index: 10;}
    
    .crosshair { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 40px; height: 40px; pointer-events: none; z-index: 2;}
    .crosshair::before, .crosshair::after { content: ""; position: absolute; background: rgba(76, 195, 138, 0.8); box-shadow: 0 0 5px rgba(76,195,138,0.8); }
    .crosshair::before { top: 50%; left: 0; width: 100%; height: 2px; transform: translateY(-50%); }
    .crosshair::after { left: 50%; top: 0; width: 2px; height: 100%; transform: translateX(-50%); }
    .crosshair-dot { position: absolute; top: 50%; left: 50%; width: 6px; height: 6px; background: var(--red); border-radius: 50%; transform: translate(-50%, -50%); z-index: 3; }

    .horizon-container { width: 200px; height: 120px; border: 2px solid var(--line); border-radius: 12px; position: relative; overflow: hidden; opacity: 0.6; pointer-events: none; background: rgba(0,0,0,0.3); transition: opacity 0.3s ease;}
    .horizon-line { position: absolute; top: 50%; left: -50%; width: 200%; height: 2px; background: var(--amber); box-shadow: 0 0 10px var(--amber); transition: transform 0.1s linear; }
    
    .debug { position: absolute; top: 8px; left: 50%; transform: translateX(-50%); display: flex; gap: 16px; padding: 8px 14px; border: 1px solid var(--line); border-radius: 8px; background: rgba(28,33,38,.9); font-size: 13px; font-variant-numeric: tabular-nums; pointer-events: none; z-index: 20;}
    .debug[hidden] { display: none; }
    .debug b { color: var(--amber); font-weight: 600; }
    .toast { position: absolute; bottom: 20px; left: 50%; transform: translateX(-50%); padding: 8px 14px; border-radius: 8px; background: var(--panel); border: 1px solid var(--amber); color: var(--amber); pointer-events: none; z-index: 20;}
    .toast[hidden] { display: none; }
    
    .btn-stabilize { position: absolute; bottom: 8vh; left: 50%; transform: translateX(-50%); width: 64px; height: 64px; border-radius: 50%; background: rgba(28,33,38,0.8); border: 2px solid var(--amber); color: var(--amber); display: flex; flex-direction: column; align-items: center; justify-content: center; z-index: 5; backdrop-filter: blur(4px); cursor: pointer; transition: opacity 0.3s ease;}
    .btn-stabilize:active { background: var(--amber); color: #000; }
    .btn-stabilize span { font-size: 24px; line-height: 1; margin-bottom: 2px; }
    .btn-stabilize small { font-size: 10px; font-weight: bold; }

    .btn-throttle-lock { position: absolute; bottom: 8vh; left: 25%; transform: translateX(-50%); width: 56px; height: 56px; border-radius: 50%; background: rgba(28,33,38,0.8); border: 2px solid var(--line); color: var(--text); display: flex; flex-direction: column; align-items: center; justify-content: center; z-index: 5; backdrop-filter: blur(4px); cursor: pointer; font-size: 12px; font-weight: bold; transition: opacity 0.3s ease;}
    .btn-throttle-lock:active { background: var(--line); }
    .btn-throttle-lock.active { border-color: var(--red); color: var(--red); box-shadow: 0 0 15px rgba(229, 72, 77, 0.4);}

    .btn-panic { position: absolute; bottom: 8vh; right: 25%; transform: translateX(50%); width: 56px; height: 56px; border-radius: 50%; background: rgba(28,33,38,0.8); border: 2px solid var(--amber); color: var(--amber); display: flex; flex-direction: column; align-items: center; justify-content: center; z-index: 5; backdrop-filter: blur(4px); cursor: pointer; font-size: 12px; font-weight: bold; transition: opacity 0.3s ease;}
    .btn-panic:active { background: var(--amber); color: #000; }

    .sticks { display: flex; align-items: flex-end; justify-content: space-between; padding: 0 var(--edge) 4vh; z-index: 10;}
    .stick { position: relative; width: var(--stick); aspect-ratio: 1; border-radius: 50%; border: 2px solid var(--line); background: linear-gradient(var(--line), var(--line)) center / 1px 70% no-repeat, linear-gradient(var(--line), var(--line)) center / 70% 1px no-repeat, var(--panel); transition: all 0.3s ease; }
    .stick::before { content: ""; position: absolute; top: 50%; left: 50%; width: calc(var(--stick) * var(--deadzone)); height: calc(var(--stick) * var(--deadzone)); transform: translate(-50%, -50%); border-radius: 50%; background: rgba(242, 169, 59, 0.2); border: 1px dashed var(--amber); pointer-events: none; z-index: 0; }
    .stick::after { content: attr(data-label); position: absolute; top: 14%; left: 50%; transform: translateX(-50%); font-size: 12px; color: var(--muted); white-space: nowrap; pointer-events: none; }
    .stick.active { border-color: var(--amber); }
    .stick.locked { border-color: var(--red); }
    .stick__knob { position: absolute; top: 50%; left: 50%; width: 36%; aspect-ratio: 1; margin: -18% 0 0 -18%; border-radius: 50%; background: radial-gradient(circle at 35% 30%, #fff, #b9c0c6); box-shadow: 0 4px 10px rgba(0,0,0,.5); pointer-events: none; will-change: transform; z-index: 1;}
    
    /* ==================================================== */
    /* MODO MINIMALISTA */
    /* ==================================================== */
    body.minimalist-ui .telemetry, body.minimalist-ui #btn-arm, body.minimalist-ui .horizon-container { opacity: 0; pointer-events: none; display: none; }
    
    body.minimalist-ui .app { grid-template-rows: 0 1fr auto; }
    body.minimalist-ui .topbar { 
        position: absolute; top: 0; left: 0; right: 0; 
        padding: 16px; pointer-events: none; 
        background: linear-gradient(to bottom, rgba(20,23,26,0.9), transparent);
    }
    
    body.minimalist-ui .menu { position: absolute; left: 16px; top: 16px; pointer-events: auto; }
    body.minimalist-ui #btn-menu { opacity: 0.7; background: rgba(28,33,38,0.6); backdrop-filter: blur(4px); border-color: transparent;}
    body.minimalist-ui .menu__panel { left: 0; right: auto; top: calc(100% + 8px); }

    body.minimalist-ui #btn-stop { 
        position: absolute; right: 16px; top: 16px; 
        width: 80px; height: 44px; pointer-events: auto; 
        opacity: 0.7; background: rgba(28,33,38,0.6); backdrop-filter: blur(4px); border-color: transparent;
    }
    body.minimalist-ui #btn-stop .hold-btn__hint { display: none; }
    body.minimalist-ui #btn-stop .hold-btn__label { font-size: 14px; }

    body.minimalist-ui .btn-stabilize, body.minimalist-ui .btn-throttle-lock, body.minimalist-ui .btn-panic { opacity: 0.2; }
    body.minimalist-ui .btn-stabilize:active, body.minimalist-ui .btn-throttle-lock:active, body.minimalist-ui .btn-panic:active { opacity: 1; }

    body.minimalist-ui .stick { opacity: 0.4; }
    body.minimalist-ui .stick.active { opacity: 0.9; }
    
    @media (max-height: 420px) { :root { --stick: calc(min(48dvh, 34vw) * var(--stick-scale)); } .hold-btn { height: 48px; } .horizon-container {width: 140px; height: 90px;} .btn-stabilize, .btn-throttle-lock, .btn-panic { width: 48px; height: 48px; bottom: 6vh; } }
    @media (orientation: portrait) { body::before { content: "Gira el dispositivo a horizontal"; position: fixed; inset: 0; z-index: 100; display: grid; place-items: center; background: var(--bg); font-size: 20px; } }
  </style>
</head>
<body>
  <div class="app">
    <header class="topbar">
      <button id="btn-arm" class="hold-btn hold-btn--arm" type="button" data-state="disarmed">
        <span class="hold-btn__label" id="arm-label">Armar</span>
        <span class="hold-btn__hint">mantener 1 s</span>
      </button>
      <section class="telemetry">
        <span class="chip" id="t-link" data-level="bad">Desconectado</span>
        <span class="chip" id="t-battery" data-level="idle">Bat –</span>
        <span class="chip" id="t-mode">Autoestable</span>
        <span class="chip chip--state" id="t-arm" data-level="idle">Desarmado</span>
      </section>
      
      <div class="menu">
        <button id="btn-menu" class="menu__toggle" type="button" aria-label="Más opciones">☰</button>
        <div id="menu-panel" class="menu__panel" hidden>
          <!-- NUEVO: Botón de Dual Rates -->
          <button type="button" class="menu__item" id="btn-rate">Modo: Normal (100%)</button>
          
          <button type="button" class="menu__item" id="btn-minimalist">Modo Minimalista</button>
          <button type="button" class="menu__item" id="btn-disarm">Desarmar (Seguro)</button>
          <button type="button" class="menu__item" id="btn-fullscreen">Pantalla Completa</button>
          <button type="button" class="menu__item" id="btn-debug">Panel debug</button>
          
          <div class="size-panel">
            <div class="size-title">Tamaño Joysticks</div>
            <div class="size-grid">
              <button class="size-btn" onclick="setStickScale(0.85, this)">S</button>
              <button class="size-btn active" onclick="setStickScale(1.0, this)">M</button>
              <button class="size-btn" onclick="setStickScale(1.15, this)">L</button>
              <button class="size-btn" onclick="setStickScale(1.3, this)">XL</button>
            </div>
          </div>

          <div class="trim-panel">
            <div class="trim-title">Ajuste Fino (Trim)</div>
            <div class="trim-grid">
              <button class="trim-btn" id="trim-p-up">P+</button>
              <button class="trim-btn" id="trim-r-left">R-</button>
              <button class="trim-btn" id="trim-r-right">R+</button>
              <button class="trim-btn" id="trim-p-down">P-</button>
              <button class="trim-btn reset" id="trim-reset">Resetear a Cero</button>
            </div>
            <div class="trim-values" id="trim-display">Pitch: 0% | Roll: 0%</div>
          </div>
        </div>
      </div>

      <button id="btn-stop" class="hold-btn hold-btn--stop" type="button">
        <span class="hold-btn__label">Parada</span>
        <span class="hold-btn__hint">mantener 0.25 s</span>
      </button>
    </header>
    
    <main class="stage" id="stage">
      <div class="crosshair"><div class="crosshair-dot"></div></div>

      <div class="horizon-container">
        <div class="horizon-line" id="horizon-line"></div>
      </div>

      <button class="btn-stabilize" id="btn-stabilize" title="Centrar Ejes de Vuelo">
        <span>⊕</span>
        <small>Nivelar</small>
      </button>

      <button class="btn-throttle-lock" id="btn-throttle-lock" title="Bloquear Altura">
        <span>🔒</span>
        <small>Gas</small>
      </button>

      <button class="btn-panic" id="btn-panic" title="Modo Estático (Detener todo)">
        <span>🛑</span>
        <small>Estático</small>
      </button>

      <aside id="debug" class="debug" hidden>
        <div>Acel <b id="d-thr">0</b> %</div><div>Guiñada <b id="d-yaw">0</b></div>
        <div>Cabeceo <b id="d-pitch">0</b></div><div>Alabeo <b id="d-roll">0</b></div>
      </aside>
      <p id="toast" class="toast" hidden></p>
    </main>
    
    <footer class="sticks">
      <div class="stick" id="stick-left" data-label="Acel / Guiñada"><div class="stick__knob"></div></div>
      <div class="stick" id="stick-right" data-label="Cabeceo / Alabeo"><div class="stick__knob"></div></div>
    </footer>
  </div>
  
  <script>
    "use strict";
    
    const controls = { throttle: 0, yaw: 0, pitch: 0, roll: 0, armed: false };
    const DEADZONE = 0.04, HOLD_MS_ARM = 1000, HOLD_MS_STOP = 250; 
    const EXPO_FACTOR = 0.3;
    const MAX_TRIM = 0.10, TRIM_STEP = 0.02;
    const SMOOTHING_FACTOR = 0.15; 
    
    let trimPitch = 0.0;
    let trimRoll = 0.0;
    let isConnected = false;
    let hasVibratedFullThrottle = false;
    
    let throttleLockActive = false;
    let lockedThrottleValue = 0;
    
    // Variables para Dual Rates y Gamepad
    let isPrecisionMode = false;
    let rateMultiplier = 1.0;
    let gamepadActive = false;
    
    const $ = (id) => document.getElementById(id);

    function vibrate(ms) { if (navigator.vibrate) navigator.vibrate(ms); }

    function sendData(type, payload) {
      if (!isConnected) return;
      payload.t = type;
      fetch('/web_rc', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) }).catch(() => {});
    }

    setInterval(() => { if (isConnected) sendData(4, {}); }, 2000);

    setInterval(async () => {
      if (!isConnected) return;
      try {
        const res = await fetch('/web_rc/status');
        const data = await res.json();
        const batPct = Math.max(0, Math.min(100, Math.round((data.voltage - 3.2) / (4.2 - 3.2) * 100)));
        $("t-battery").textContent = `Bat ${batPct}%`;
        $("t-battery").dataset.level = batPct < 20 ? "bad" : batPct < 40 ? "warn" : "ok";
        document.body.classList.toggle("critical-battery", batPct < 15);
      } catch(e) {}
    }, 1000);

    window.setStickScale = function(scale, btnElement) {
      document.documentElement.style.setProperty('--stick-scale', scale);
      document.querySelectorAll('.size-btn').forEach(btn => btn.classList.remove('active'));
      btnElement.classList.add('active');
      setTimeout(() => { leftStick.refresh(); rightStick.refresh(); }, 250);
    };

    function applyExpo(val) { return (1 - EXPO_FACTOR) * val + EXPO_FACTOR * (val * val * val); }

    function updateHorizon(roll, pitch) {
        const line = $("horizon-line");
        const angle = roll * -30;
        const offsetY = pitch * -40;
        line.style.transform = `translateY(${offsetY}px) rotate(${angle}deg)`;
    }

    function sendStickData() {
        // Aplicamos Expo y luego el Multiplicador de Precisión (Dual Rates)
        let outRoll = applyExpo(rightStick.outputValue.x) * rateMultiplier;
        let outPitch = applyExpo(rightStick.outputValue.y) * rateMultiplier;
        let outYaw = leftStick.outputValue.x * rateMultiplier; // Yaw también baja en precisión
        
        outRoll = Math.max(-1, Math.min(1, outRoll + trimRoll));
        outPitch = Math.max(-1, Math.min(1, outPitch + trimPitch));

        let currentThrottle = throttleLockActive ? lockedThrottleValue : controls.throttle;

        sendData(1, { 
            th: Math.round((currentThrottle * 2 - 1) * 100), 
            y: Math.round(outYaw * 100), 
            r: Math.round(outRoll * 100), 
            p: Math.round(outPitch * 100) 
        });
        
        updateHorizon(outRoll, outPitch);
        updateDebug(outRoll, outPitch, currentThrottle);
        
        if (currentThrottle > 0.98 && !hasVibratedFullThrottle) { vibrate(50); hasVibratedFullThrottle = true; } 
        else if (currentThrottle < 0.90) { hasVibratedFullThrottle = false; }
    }

    setInterval(() => {
        if (isConnected) sendStickData();
    }, 50);

    function createJoystick(el, { springY, onChange }) {
      const knob = el.querySelector(".stick__knob");
      let touchId = null, rect = null;
      let value = { x: 0, y: 0 };
      let outputValue = { x: 0, y: 0 };
      let animationFrame = null;

      function render() {
        const radius = rect ? rect.width / 2 : el.clientWidth / 2;
        const travel = radius - knob.offsetWidth / 2;
        knob.style.transform = `translate(${value.x * travel}px, ${-value.y * travel}px)`;
      }

      function loop() {
        outputValue.x += (value.x - outputValue.x) * SMOOTHING_FACTOR;
        outputValue.y += (value.y - outputValue.y) * SMOOTHING_FACTOR;

        if (Math.abs(value.x - outputValue.x) < 0.001) outputValue.x = value.x;
        if (Math.abs(value.y - outputValue.y) < 0.001) outputValue.y = value.y;

        animationFrame = requestAnimationFrame(loop);
      }
      
      loop();

      function update(touch) {
        const radius = rect.width / 2, travel = radius - knob.offsetWidth / 2;
        let dx = touch.clientX - (rect.left + radius), dy = touch.clientY - (rect.top + radius);
        const dist = Math.hypot(dx, dy);
        if (dist > travel) { dx = (dx / dist) * travel; dy = (dy / dist) * travel; }
        
        let targetX = dx / travel;
        let targetY = -dy / travel;
        
        value.x = Math.abs(targetX) < DEADZONE ? 0 : targetX;
        
        if (el.id === "stick-left" && throttleLockActive) {
            // Ignorar Y
        } else {
            value.y = Math.abs(targetY) < DEADZONE ? 0 : targetY;
        }
        
        render();
        onChange(value.x, value.y);
      }

      function findTouch(list) { for (const t of list) if (t.identifier === touchId) return t; return null; }
      
      el.addEventListener("touchstart", (e) => {
        e.preventDefault(); if (touchId !== null) return;
        touchId = e.changedTouches[0].identifier;
        rect = el.getBoundingClientRect(); el.classList.add("active"); update(e.changedTouches[0]);
      }, { passive: false });
      
      el.addEventListener("touchmove", (e) => { e.preventDefault(); const t = findTouch(e.changedTouches); if (t) update(t); }, { passive: false });
      
      function end(e) {
        e.preventDefault(); if (!findTouch(e.changedTouches)) return;
        touchId = null; el.classList.remove("active");
        value.x = 0; 
        if (springY && !(el.id === "stick-left" && throttleLockActive)) { value.y = 0; }
        render(); onChange(value.x, value.y);
      }
      
      el.addEventListener("touchend", end, { passive: false });
      el.addEventListener("touchcancel", end, { passive: false });
      
      return { 
        get outputValue() { return outputValue; }, 
        resetY() { value.y = 0; render(); }, 
        resetX() { value.x = 0; render(); }, 
        refresh() { rect = el.getBoundingClientRect(); render(); },
        setPosition(x, y) { 
          value.x = x; value.y = y; 
          outputValue.x = x; outputValue.y = y; 
          render(); 
        },
        // NUEVO: Actualizar desde Gamepad
        updateGamepad(x, y) {
            let targetX = x;
            let targetY = y;
            value.x = Math.abs(targetX) < 0.1 ? 0 : targetX; // Deadzone más grande para gamepad (0.1)
            if (el.id === "stick-left" && throttleLockActive) {
                // Ignorar Y
            } else {
                value.y = Math.abs(targetY) < 0.1 ? 0 : targetY;
            }
            render();
            onChange(value.x, value.y);
        }
      };
    }

    const leftStick = createJoystick($("stick-left"), {
      springY: false,
      onChange(x, y) {
        controls.yaw = x;
        if (!throttleLockActive) {
            controls.throttle = controls.armed ? (y + 1) / 2 : 0;
        }
      }
    });

    const rightStick = createJoystick($("stick-right"), {
      springY: true,
      onChange(x, y) {
        controls.roll = x; controls.pitch = y;
      }
    });

    // --- SOPORTE DE GAMEPAD (BLUETOOTH/USB) ---
    function pollGamepad() {
        const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
        if (!gamepads || !gamepads[0]) {
            gamepadActive = false;
            requestAnimationFrame(pollGamepad);
            return;
        }
        
        const gp = gamepads[0];
        if (!gamepadActive) {
            toast("Gamepad Conectado");
            gamepadActive = true;
        }
        
        // Mapeo estándar: 0=Izq X, 1=Izq Y, 2=Der X, 3=Der Y
        let lx = gp.axes[0] || 0;
        let ly = gp.axes[1] || 0;
        let rx = gp.axes[2] || 0;
        let ry = gp.axes[3] || 0;
        
        // Invertir Y para que arriba sea positivo en la lógica del dron
        // (En gamepads, arriba es -1. En nuestro joystick, arriba es +1)
        leftStick.updateGamepad(lx, -ly);
        rightStick.updateGamepad(rx, -ry);
        
        requestAnimationFrame(pollGamepad);
    }
    requestAnimationFrame(pollGamepad);

    // --- BOTÓN DUAL RATES (PRECISIÓN) ---
    $("btn-rate").addEventListener("click", () => {
        isPrecisionMode = !isPrecisionMode;
        rateMultiplier = isPrecisionMode ? 0.6 : 1.0; // 60% de sensibilidad en precisión
        
        const btn = $("btn-rate");
        if (isPrecisionMode) {
            btn.textContent = "Modo: Precisión (60%)";
            btn.classList.add("active-mode");
            vibrate([30, 30]);
        } else {
            btn.textContent = "Modo: Normal (100%)";
            btn.classList.remove("active-mode");
            vibrate(30);
        }
        $("menu-panel").hidden = true;
    });

    // --- AUTO-DESARMAR POR PÉRDIDA DE FOCO ---
    document.addEventListener("visibilitychange", () => {
        if (document.hidden && controls.armed) {
            emergencyStop();
            toast("PÉRDIDA DE FOCO: Dron Detenido");
        }
    });

    $("btn-panic").addEventListener("click", () => {
        if (!controls.armed) { toast("Arma el dron primero"); return; }
        rightStick.setPosition(0, 0);
        leftStick.resetX(); 
        throttleLockActive = true;
        lockedThrottleValue = controls.throttle;
        $("btn-throttle-lock").classList.add("active");
        $("stick-left").classList.add("locked");
        vibrate([100, 50, 100, 50, 100]);
        toast("MODO ESTÁTICO: Dron Detenido");
        sendStickData();
    });

    $("btn-throttle-lock").addEventListener("click", () => {
        if (!controls.armed) { toast("Arma el dron primero"); return; }
        throttleLockActive = !throttleLockActive;
        if (throttleLockActive) {
            lockedThrottleValue = controls.throttle;
            $("btn-throttle-lock").classList.add("active");
            $("stick-left").classList.add("locked");
            vibrate([50, 50]);
            toast(`Gas Fijado al ${Math.round(lockedThrottleValue * 100)}%`);
        } else {
            $("btn-throttle-lock").classList.remove("active");
            $("stick-left").classList.remove("locked");
            vibrate(30);
            toast("Gas Libre");
        }
    });

    $("btn-stabilize").addEventListener("click", () => {
        if (!controls.armed) { toast("Arma el dron primero"); return; }
        rightStick.setPosition(0, 0);
        trimPitch = 0;
        trimRoll = 0;
        updateTrimUI();
        sendStickData();
        vibrate(100);
        toast("Ejes nivelados");
    });

    function bindHoldButton(btn, { ms, canStart = () => true, onConfirm }) {
      let touchId = null, timer = null;
      function start() {
        if (!canStart()) { btn.classList.add("denied"); setTimeout(() => btn.classList.remove("denied"), 300); return false; }
        btn.style.setProperty("--hold-ms", ms + "ms"); btn.classList.add("holding");
        timer = setTimeout(() => { timer = null; btn.classList.remove("holding"); btn.classList.add("done"); setTimeout(() => btn.classList.remove("done"), 350); onConfirm(); }, ms);
        return true;
      }
      function cancel() { clearTimeout(timer); timer = null; touchId = null; btn.classList.remove("holding"); }
      btn.addEventListener("touchstart", (e) => { e.preventDefault(); if (touchId !== null) return; if (start()) touchId = e.changedTouches[0].identifier; }, { passive: false });
      const endHandler = (e) => { e.preventDefault(); for (const t of e.changedTouches) if (t.identifier === touchId) { cancel(); return; } };
      btn.addEventListener("touchend", endHandler, { passive: false });
      btn.addEventListener("touchcancel", endHandler, { passive: false });
    }

    bindHoldButton($("btn-arm"), {
      ms: HOLD_MS_ARM,
      canStart: () => {
        if (controls.armed) { toast("Ya está armado"); return false; }
        if (controls.throttle > 0.05) { toast("Baja el acelerador"); return false; }
        return true;
      },
      onConfirm() { 
          setArmed(true); vibrate([50, 50, 50]); 
          sendData(2, { b: 0, s: 1 }); setTimeout(() => sendData(2, { b: 0, s: 0 }), 200); 
      }
    });

    bindHoldButton($("btn-stop"), {
      ms: HOLD_MS_STOP,
      onConfirm() { 
          emergencyStop(); vibrate([200, 100, 200]);
          sendData(2, { b: 2, s: 1 }); setTimeout(() => sendData(2, { b: 2, s: 0 }), 200); 
      }
    });

    function setArmed(on) {
      controls.armed = on;
      $("btn-arm").dataset.state = on ? "armed" : "disarmed";
      $("arm-label").textContent = on ? "Armado" : "Armar";
      $("t-arm").textContent = on ? "Armado" : "Desarmado";
      $("t-arm").dataset.level = on ? "warn" : "idle";
      if (!on) { 
          controls.throttle = 0; 
          leftStick.resetY(); 
          if (throttleLockActive) {
              throttleLockActive = false;
              $("btn-throttle-lock").classList.remove("active");
              $("stick-left").classList.remove("locked");
          }
          sendStickData(); 
      }
    }

    function emergencyStop() { setArmed(false); toast("PARADA DE EMERGENCIA"); }

    function updateTrimUI() {
        $("trim-display").textContent = `Pitch: ${Math.round(trimPitch*100)}% | Roll: ${Math.round(trimRoll*100)}%`;
    }

    $("trim-p-up").addEventListener("click", () => { trimPitch = Math.min(MAX_TRIM, trimPitch + TRIM_STEP); updateTrimUI(); vibrate(20); });
    $("trim-p-down").addEventListener("click", () => { trimPitch = Math.max(-MAX_TRIM, trimPitch - TRIM_STEP); updateTrimUI(); vibrate(20); });
    $("trim-r-right").addEventListener("click", () => { trimRoll = Math.min(MAX_TRIM, trimRoll + TRIM_STEP); updateTrimUI(); vibrate(20); });
    $("trim-r-left").addEventListener("click", () => { trimRoll = Math.max(-MAX_TRIM, trimRoll - TRIM_STEP); updateTrimUI(); vibrate(20); });
    $("trim-reset").addEventListener("click", () => { trimPitch = 0; trimRoll = 0; updateTrimUI(); vibrate(30); });

    $("btn-menu").addEventListener("click", (e) => { e.stopPropagation(); $("menu-panel").hidden = !$("menu-panel").hidden; });
    document.addEventListener("touchstart", (e) => { if (!$("menu-panel").hidden && !e.target.closest(".menu")) $("menu-panel").hidden = true; }, { passive: true });
    
    $("btn-disarm").addEventListener("click", () => { 
        setArmed(false); sendData(2, { b: 1, s: 1 }); setTimeout(() => sendData(2, { b: 1, s: 0 }), 200); 
        $("menu-panel").hidden = true; 
    });

    $("btn-minimalist").addEventListener("click", () => {
        document.body.classList.toggle("minimalist-ui");
        $("menu-panel").hidden = true;
        toast(document.body.classList.contains("minimalist-ui") ? "Modo Minimalista ON" : "Modo Normal");
    });

    $("btn-fullscreen").addEventListener("click", () => {
        if (!document.fullscreenElement) { document.documentElement.requestFullscreen().catch(() => toast("No soportado")); } 
        else { document.exitFullscreen(); }
        $("menu-panel").hidden = true;
    });

    $("btn-debug").addEventListener("click", () => { $("debug").hidden = !$("debug").hidden; $("menu-panel").hidden = true; });

    function updateDebug(outRoll = 0, outPitch = 0, currentThrottle = 0) {
      if ($("debug").hidden) return;
      $("d-thr").textContent = Math.round(currentThrottle * 100);
      $("d-yaw").textContent = leftStick.outputValue.x.toFixed(2);
      $("d-pitch").textContent = outPitch.toFixed(2);
      $("d-roll").textContent = outRoll.toFixed(2);
    }
    
    function toast(msg) { const t = $("toast"); t.textContent = msg; t.hidden = false; setTimeout(() => t.hidden = true, 2000); }

    setTimeout(() => {
      isConnected = true;
      $("t-link").textContent = "Conectado";
      $("t-link").dataset.level = "ok";
      updateTrimUI();
      updateHorizon(0, 0);
      leftStick.refresh(); rightStick.refresh();
    }, 1000);
    
    window.addEventListener("resize", () => { leftStick.refresh(); rightStick.refresh(); });
  </script>
</body>
</html>
)rawliteral";

#endif