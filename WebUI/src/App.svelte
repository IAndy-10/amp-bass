<script>
  import { derived } from 'svelte/store';
  import { onMount } from 'svelte';
  import NeuKnob from './components/NeuKnob.svelte';
  import NeuKnobDiscrete from './components/NeuKnobDiscrete.svelte';
  import NeuButton from './components/NeuButton.svelte';
  import NeuSlider from './components/NeuSlider.svelte';
  import Tooltip from './components/Tooltip.svelte';

  import { sliders, toggles, combos } from './state/store';
  import { setSliderValue, sliderDragStarted, sliderDragEnded, setToggleValue, setComboIndex } from './bridge/bridge';

  const DESIGN_W = 1350;
  const DESIGN_H = 460;

  let sceneEl;
  let scale = 1;

  function rescale() {
    const sw = window.innerWidth / DESIGN_W;
    const sh = window.innerHeight / DESIGN_H;
    scale = Math.min(sw, sh, 1);
  }

  onMount(() => {
    rescale();
    window.addEventListener('resize', rescale);
    return () => window.removeEventListener('resize', rescale);
  });

  // Destructure stores into top-level const so Svelte's $-prefix auto-subscribe works.
  // In Svelte, $foo.bar means "subscribe to store foo, then access .bar" — NOT "access bar on object foo".
  // So we need each store as its own variable.

  // Slider stores
  const gain = sliders.gain;
  const tubeGain = sliders.tubeGain;
  const bass = sliders.bass;
  const midrange = sliders.midrange;
  const treble = sliders.treble;
  const master = sliders.master;
  const geqLevel = sliders.geqLevel;
  const dryWet = sliders.dryWet;

  // Toggle stores
  const bright = toggles.bright;
  const ultraHi = toggles.ultraHi;
  const ultraLo = toggles.ultraLo;
  const inputPad = toggles.inputPad;
  const inputHiLo = toggles.inputHiLo;
  const geqOn = toggles.geqOn;

  // Combo stores
  const midFreq = combos.midFreq;

  // ── GEQ band mapping ──
  /** @type {import('./types/parameters').SliderParamId[]} */
  const geqIds = ['geq33','geq80','geq150','geq300','geq600','geq900','geq2k','geq5k','geq8k'];
  const eqLabels = ['33', '80', '150', '300', '600', '900', '2K', '5K', '8K'];
  const geqRanges = [
    [-15, 15], [-8, 8], [-8, 8], [-8, 8], [-8, 8],
    [-8, 8], [-8, 8], [-9, 9], [-10, 10]
  ];

  // Derived store combining all GEQ band values into an array for the {#each} loop.
  const geqValues = derived(
    [sliders.geq33, sliders.geq80, sliders.geq150, sliders.geq300, sliders.geq600,
     sliders.geq900, sliders.geq2k, sliders.geq5k, sliders.geq8k],
    vals => vals
  );
</script>

<Tooltip />

<div class="scene" bind:this={sceneEl}>
  <div class="panel" style="transform: scale({scale}); transform-origin: center center;">

    <!-- Header -->
    <div class="header">
      <div class="brand-area">
        <div class="brand-script">Ampeg</div>
        <div class="brand-model">SVT-3 PRO</div>
      </div>
      <div class="indicator-led led-on"></div>
    </div>

    <div class="rule"></div>

    <!-- Main body -->
    <div class="body">

      <!-- PREAMP SECTION -->
      <div class="section preamp-section">
        <div class="section-inner">
          <div class="toggle-col">
            <NeuButton label="BRT" active={$bright} led={true}
              on:change={e => setToggleValue('bright', e.detail.active)} />
            <NeuButton label="Ultra HI" active={$ultraHi} led={true}
              on:change={e => setToggleValue('ultraHi', e.detail.active)} />
            <NeuButton label="Ultra LO" active={$ultraLo} led={true}
              on:change={e => setToggleValue('ultraLo', e.detail.active)} />
          </div>

          <div class="knob-stack">
            <NeuKnob label="GAIN" value={$gain} min={0} max={10} size="large" prominent={true}
              on:dragstart={() => sliderDragStarted('gain')}
              on:change={e => setSliderValue('gain', e.detail.value)}
              on:dragend={() => sliderDragEnded('gain')} />
            <div class="peak-led-row">
              <span class="mini-led red"></span>
              <span class="mini-label">PEAK/MUTE</span>
            </div>
          </div>

          <div class="pad-switches">
            <NeuButton label="-15dB" active={$inputPad} led={true}
              on:change={e => setToggleValue('inputPad', e.detail.active)} />
            <NeuButton label="HI/LO" active={$inputHiLo} led={true}
              on:change={e => setToggleValue('inputHiLo', e.detail.active)} />
          </div>
        </div>
        <div class="section-label">PREAMP</div>
      </div>

      <div class="vsep"></div>

      <!-- TONE SECTION -->
      <div class="section tone-section">
        <div class="section-inner">
          <NeuKnob label="BASS" value={$bass} min={0} max={10}
            on:dragstart={() => sliderDragStarted('bass')}
            on:change={e => setSliderValue('bass', e.detail.value)}
            on:dragend={() => sliderDragEnded('bass')} />
          <NeuKnob label="MIDRANGE" value={$midrange} min={0} max={10}
            on:dragstart={() => sliderDragStarted('midrange')}
            on:change={e => setSliderValue('midrange', e.detail.value)}
            on:dragend={() => sliderDragEnded('midrange')} />
          <NeuKnobDiscrete label="FREQUENCY" options={['220','450','800','1.6k','3k']} selected={$midFreq}
            on:change={e => setComboIndex('midFreq', e.detail.selected)} />
          <NeuKnob label="TREBLE" value={$treble} min={0} max={10}
            on:dragstart={() => sliderDragStarted('treble')}
            on:change={e => setSliderValue('treble', e.detail.value)}
            on:dragend={() => sliderDragEnded('treble')} />
        </div>
        <div class="section-label">TONE</div>
      </div>

      <div class="vsep"></div>

      <!-- MASTER / TUBE SECTION -->
      <div class="section master-section">
        <div class="section-inner">
          <NeuKnob label="MASTER" value={$master} min={0} max={10} size="large" prominent={true}
            on:dragstart={() => sliderDragStarted('master')}
            on:change={e => setSliderValue('master', e.detail.value)}
            on:dragend={() => sliderDragEnded('master')} />
          <NeuKnob label="TUBE GAIN" value={$tubeGain} min={0} max={10}
            on:dragstart={() => sliderDragStarted('tubeGain')}
            on:change={e => setSliderValue('tubeGain', e.detail.value)}
            on:dragend={() => sliderDragEnded('tubeGain')} />

          <div class="toggle-col">
            <NeuButton label="EQ" active={$geqOn} led={true}
              on:change={e => setToggleValue('geqOn', e.detail.active)} />
          </div>
        </div>
        <div class="section-label">POWER AMP</div>
      </div>

      <div class="vsep"></div>

      <!-- GRAPHIC EQ SECTION -->
      <div class="section eq-section">
        <div class="eq-header">
          <span class="eq-range">+15dB</span>
          <span class="eq-title">GRAPHIC EQ</span>
          <span class="eq-range">-15dB</span>
        </div>
        <div class="eq-sliders">
          {#each geqIds as id, i}
            <NeuSlider label={eqLabels[i]} value={$geqValues[i]} min={geqRanges[i][0]} max={geqRanges[i][1]} unit="dB"
              disabled={!$geqOn}
              on:dragstart={() => sliderDragStarted(id)}
              on:change={e => setSliderValue(id, e.detail.value)}
              on:dragend={() => sliderDragEnded(id)} />
          {/each}
        </div>
      </div>

      <div class="vsep"></div>

      <!-- OUTPUT SECTION -->
      <div class="section output-section">
        <div class="section-inner output-inner">
          <div class="output-labels">
            <span class="eq-range">+8dB</span>
            <span class="eq-range">-10dB</span>
          </div>
          <NeuSlider label="LEVEL" value={$geqLevel} min={-10} max={10} unit="dB"
            disabled={!$geqOn}
            on:dragstart={() => sliderDragStarted('geqLevel')}
            on:change={e => setSliderValue('geqLevel', e.detail.value)}
            on:dragend={() => sliderDragEnded('geqLevel')} />
          <NeuKnob label="DRY/WET" value={$dryWet} min={0} max={100} unit="%"
            formatFn={v => v.toFixed(0)}
            on:dragstart={() => sliderDragStarted('dryWet')}
            on:change={e => setSliderValue('dryWet', e.detail.value)}
            on:dragend={() => sliderDragEnded('dryWet')} />
        </div>
        <div class="section-label">OUTPUT</div>
      </div>

    </div>

    <!-- Footer -->
    <div class="rule"></div>
    <div class="footer">
      <span>AMPEG SVT-3 PRO · Bass Amplifier</span>
      <span>v0.1.0</span>
    </div>

  </div>
</div>

<style>
  @import url('https://fonts.googleapis.com/css2?family=DM+Sans:ital,wght@0,200;0,300;0,400;0,500;1,200&family=DM+Serif+Display:ital@0;1&display=swap');

  :global(body) {
    margin: 0;
    padding: 0;
    overflow: hidden;
  }

  .scene {
    width: 100vw;
    height: 100vh;
    background:
      radial-gradient(ellipse at 25% 35%, rgba(60,55,48,0.4), transparent 55%),
      radial-gradient(ellipse at 75% 70%, rgba(50,45,38,0.3), transparent 50%),
      #2a2520;
    display: flex;
    align-items: center;
    justify-content: center;
    overflow: hidden;
    font-family: 'DM Sans', sans-serif;
  }

  .panel {
    background:
      radial-gradient(ellipse at 30% 40%, rgba(80,72,62,0.5), transparent 60%),
      #3d3632;
    border-radius: 18px;
    box-shadow:
      14px 14px 34px rgba(0,0,0,0.5),
      -4px -4px 14px rgba(80,72,62,0.15),
      inset 0 1px 0 rgba(100,90,78,0.25);
    padding: 24px 28px;
    width: max-content;
    /* Textured metal feel */
    background-image:
      url("data:image/svg+xml,%3Csvg viewBox='0 0 4 4' xmlns='http://www.w3.org/2000/svg'%3E%3Crect width='1' height='1' fill='rgba(255,255,255,0.015)'/%3E%3C/svg%3E");
    background-size: 3px 3px;
  }

  /* ── Header ── */
  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 16px;
  }

  .brand-area {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    flex: 1;
    text-align: center;
  }

  .brand-script {
    font-family: 'DM Serif Display', serif;
    font-style: italic;
    font-size: 1.8rem;
    letter-spacing: 0.08em;
    color: rgba(220, 210, 195, 0.85);
    text-shadow: 0 1px 2px rgba(0,0,0,0.4);
  }

  .brand-model {
    font-size: 0.72rem;
    font-weight: 400;
    letter-spacing: 0.28em;
    color: rgba(200, 190, 170, 0.6);
    text-transform: uppercase;
  }

  .indicator-led {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: rgba(80, 70, 55, 0.5);
    transition: background 0.3s, box-shadow 0.3s;
    position: absolute;
    right: 40px;
    top: 36px;
  }

  .indicator-led.led-on {
    background: #4ade80;
    box-shadow: 0 0 8px rgba(74, 222, 128, 0.6);
  }

  /* ── Rules ── */
  .rule {
    height: 1px;
    background: linear-gradient(to right,
      transparent,
      rgba(120,108,90,0.25) 20%,
      rgba(120,108,90,0.25) 80%,
      transparent
    );
    margin: 0 -4px 16px;
  }

  /* ── Body ── */
  .body {
    display: flex;
    align-items: flex-start;
    gap: 0;
  }

  .section {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 12px;
  }

  .section-inner {
    display: flex;
    align-items: flex-end;
    gap: 16px;
  }

  .section-label {
    font-size: 0.5rem;
    font-weight: 300;
    letter-spacing: 0.22em;
    text-transform: uppercase;
    color: rgba(180, 165, 140, 0.4);
    margin-top: 4px;
  }

  .vsep {
    width: 1px;
    align-self: stretch;
    background: linear-gradient(
      to bottom,
      transparent,
      rgba(120,108,90,0.2) 30%,
      rgba(120,108,90,0.2) 70%,
      transparent
    );
    margin: 0 6px;
  }

  /* ── Preamp specifics ── */
  .preamp-section {
    padding: 0 8px;
  }

  .knob-stack {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
  }

  .peak-led-row {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .mini-led {
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: rgba(80, 70, 55, 0.4);
  }

  .mini-led.red {
    background: rgba(180, 60, 60, 0.3);
  }

  .mini-label {
    font-size: 0.42rem;
    font-weight: 300;
    letter-spacing: 0.1em;
    color: rgba(180, 165, 140, 0.45);
    text-transform: uppercase;
  }

  .pad-switches {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  /* ── Tone section ── */
  .tone-section {
    padding: 0 8px;
  }

  /* ── Master section ── */
  .master-section {
    padding: 0 8px;
  }

  .toggle-col {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  /* ── Graphic EQ section ── */
  .eq-section {
    padding: 0 8px;
    flex: 1;
  }

  .eq-header {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 2px;
    width: 100%;
    padding-right: 4px;
  }

  .eq-title {
    font-size: 0.5rem;
    font-weight: 300;
    letter-spacing: 0.18em;
    color: rgba(180, 165, 140, 0.5);
    text-transform: uppercase;
    align-self: center;
    margin-bottom: 4px;
  }

  .eq-range {
    font-size: 0.42rem;
    font-weight: 200;
    letter-spacing: 0.08em;
    color: rgba(180, 165, 140, 0.35);
  }

  .eq-sliders {
    display: flex;
    gap: 6px;
    justify-content: center;
  }

  /* ── Output section ── */
  .output-section {
    padding: 0 8px;
  }

  .output-inner {
    flex-direction: column;
    align-items: center;
    gap: 12px;
  }

  .output-labels {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 80px;
  }

  /* ── Footer ── */
  .footer {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-top: 14px;
    font-size: 0.48rem;
    font-weight: 200;
    letter-spacing: 0.2em;
    text-transform: uppercase;
    color: rgba(180, 165, 140, 0.3);
  }

  /* ── Override neumorphic component colors for dark panel ── */
  :global(.wrap .label) {
    color: rgba(200, 185, 160, 0.65) !important;
  }

  :global(.wrap .val) {
    color: rgba(200, 185, 160, 0.5) !important;
  }

  :global(.knob) {
    background: #4a4038 !important;
    box-shadow:
      5px 5px 12px rgba(0,0,0,0.4),
      -4px -4px 10px rgba(80,72,62,0.25) !important;
  }

  :global(.knob:hover:not(.pressed)) {
    box-shadow:
      4px 4px 10px rgba(0,0,0,0.35),
      -5px -5px 12px rgba(90,80,68,0.3) !important;
  }

  :global(.knob.pressed) {
    box-shadow:
      2px 2px 5px rgba(0,0,0,0.45),
      -2px -2px 4px rgba(70,62,52,0.2),
      inset 3px 3px 7px rgba(0,0,0,0.3),
      inset -2px -2px 5px rgba(80,72,62,0.2) !important;
  }

  :global(.knob .inner) {
    background: radial-gradient(circle at 35% 30%, #5a5048, #3e3630) !important;
    box-shadow:
      3px 3px 7px rgba(0,0,0,0.35),
      -2px -2px 5px rgba(80,72,62,0.25) !important;
  }

  :global(.knob .marker) {
    background: rgba(220, 210, 190, 0.6) !important;
  }

  :global(.knob .arc circle:first-child) {
    stroke: rgba(100, 90, 75, 0.25) !important;
  }

  :global(.knob .arc circle:last-child) {
    stroke: rgba(200, 185, 155, 0.4) !important;
  }

  :global(.btn) {
    background: #4a4038 !important;
    box-shadow:
      5px 5px 12px rgba(0,0,0,0.4),
      -4px -4px 10px rgba(80,72,62,0.25) !important;
  }

  :global(.btn.active) {
    background: #3e3630 !important;
    box-shadow:
      inset 4px 4px 8px rgba(0,0,0,0.35),
      inset -3px -3px 6px rgba(70,62,52,0.2) !important;
  }

  :global(.groove) {
    background: #3a332e !important;
    box-shadow:
      inset 3px 3px 7px rgba(0,0,0,0.4),
      inset -2px -2px 5px rgba(70,62,52,0.15) !important;
  }

  :global(.fill) {
    background: linear-gradient(to top, rgba(200,185,155,0.4), rgba(160,145,120,0.2)) !important;
  }

  :global(.thumb) {
    background: radial-gradient(ellipse at 40% 30%, #5a5048, #3e3630) !important;
    box-shadow:
      3px 3px 7px rgba(0,0,0,0.4),
      -2px -2px 5px rgba(80,72,62,0.25) !important;
  }

  :global(.thumb::after) {
    background: rgba(200,185,160,0.35) !important;
  }

  /* LED overrides for dark theme */
  :global(.led.led-on) {
    background: #4ade80 !important;
    box-shadow: 0 0 6px rgba(74, 222, 128, 0.6) !important;
  }

  :global(.icon) {
    color: rgba(200, 185, 160, 0.5) !important;
  }

  :global(.btn.active .icon) {
    color: rgba(220, 210, 190, 0.75) !important;
  }

  :global(.pill-label) {
    color: rgba(200, 185, 160, 0.6) !important;
  }

  /* Discrete knob dropdown override */
  :global(.trigger) {
    background: #4a4038 !important;
    box-shadow:
      5px 5px 12px rgba(0,0,0,0.4),
      -4px -4px 10px rgba(80,72,62,0.25) !important;
  }

  :global(.trigger-text) {
    color: rgba(220, 210, 190, 0.8) !important;
  }

  :global(.chevron) {
    color: rgba(200, 185, 160, 0.4) !important;
  }

  :global(.dropdown) {
    background: #4a4038 !important;
    box-shadow:
      8px 8px 20px rgba(0,0,0,0.5),
      -4px -4px 12px rgba(80,72,62,0.2) !important;
  }

  :global(.option) {
    color: rgba(200, 185, 160, 0.65) !important;
  }

  :global(.option:hover:not(.active)) {
    background: #554c42 !important;
    box-shadow:
      3px 3px 6px rgba(0,0,0,0.3),
      -2px -2px 4px rgba(80,72,62,0.2) !important;
    color: rgba(220, 210, 190, 0.85) !important;
  }

  :global(.option.active) {
    background: #3a332e !important;
    box-shadow:
      inset 3px 3px 6px rgba(0,0,0,0.35),
      inset -2px -2px 4px rgba(60,52,44,0.15) !important;
    color: rgba(220, 210, 190, 0.9) !important;
  }

  /* Context menu override */
  :global(.ctx-menu) {
    background: #4a4038 !important;
    box-shadow:
      8px 8px 20px rgba(0,0,0,0.5),
      -4px -4px 12px rgba(80,72,62,0.2) !important;
  }

  :global(.ctx-item) {
    color: rgba(200, 185, 160, 0.7) !important;
  }

  :global(.ctx-item:hover) {
    background: #554c42 !important;
    color: rgba(220, 210, 190, 0.9) !important;
  }
</style>
