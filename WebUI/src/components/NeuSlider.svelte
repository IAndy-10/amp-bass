<script>
  import { createEventDispatcher } from 'svelte';

  export let label = 'LEVEL';
  export let value = 0.5;
  export let min = 0;
  export let max = 100;
  export let unit = '';
  export let disabled = false;   // greyed out and ignores input

  const dispatch = createEventDispatcher();

  /** @type {HTMLDivElement | undefined} */
  let trackEl;
  let dragging = false;

  $: displayVal = (min + value * (max - min)).toFixed(0);

  /** @param {MouseEvent} e */
  function onMouseDown(e) {
    if (disabled) return;
    dragging = true;
    dispatch('dragstart');
    update(e);
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  }
  /** @param {MouseEvent} e */
  function onMove(e) {
    if (!dragging) return;
    update(e);
    dispatch('change', { value });
  }
  /** @param {MouseEvent} e */
  function update(e) {
    if (!trackEl) return;
    const rect = trackEl.getBoundingClientRect();
    value = Math.min(1, Math.max(0, 1 - (e.clientY - rect.top) / rect.height));
  }
  function onUp() {
    dragging = false;
    dispatch('change', { value });
    dispatch('dragend');
    window.removeEventListener('mousemove', onMove);
    window.removeEventListener('mouseup', onUp);
  }
  // If disabled mid-drag (e.g. GEQ switched off), end the gesture cleanly.
  $: if (disabled && dragging) onUp();

  /** @param {KeyboardEvent} e */
  function onKeyDown(e) {
    if (disabled) return;
    const step = e.shiftKey ? 0.1 : 0.01;
    let next = value;
    if (e.key === 'ArrowUp' || e.key === 'ArrowRight') next = value + step;
    else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') next = value - step;
    else if (e.key === 'Home') next = 0;
    else if (e.key === 'End') next = 1;
    else return;
    e.preventDefault();
    value = Math.min(1, Math.max(0, next));
    dispatch('dragstart');
    dispatch('change', { value });
    dispatch('dragend');
  }
  /** @param {WheelEvent} e */
  function onWheel(e) {
    if (disabled) return;
    e.preventDefault();
    value = Math.min(1, Math.max(0, value - e.deltaY * 0.001));
    dispatch('change', { value });
  }
</script>

<div class="wrap" class:disabled>
  <div class="label">{label}</div>

  <div class="track" bind:this={trackEl}
    role="slider" tabindex={disabled ? -1 : 0}
    aria-label={label} aria-orientation="vertical" aria-disabled={disabled}
    aria-valuemin={min} aria-valuemax={max} aria-valuenow={Number(displayVal)}
    aria-valuetext="{displayVal}{unit}"
    on:mousedown={onMouseDown} on:wheel={onWheel} on:keydown={onKeyDown}>
    <!-- Inset groove -->
    <div class="groove">
      <div class="fill" style="height: {value * 100}%"></div>
    </div>
    <!-- Thumb -->
    <div class="thumb" class:pressed={dragging} style="bottom: calc({value * 100}% - 13px)"></div>
  </div>

  <div class="val">{displayVal}{unit}</div>
</div>

<style>
  @import url('https://fonts.googleapis.com/css2?family=DM+Sans:wght@200;300;400&display=swap');

  .wrap {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 7px;
    user-select: none;
  }

  .wrap.disabled {
    opacity: 0.4;
    filter: grayscale(0.6);
    transition: opacity 0.15s;
  }

  .wrap.disabled .track,
  .wrap.disabled .thumb {
    cursor: not-allowed;
  }

  .label {
    font-family: 'DM Sans', sans-serif;
    font-size: 0.58rem;
    font-weight: 300;
    letter-spacing: 0.18em;
    text-transform: uppercase;
    color: rgba(130, 110, 85, 0.7);
  }

  .track {
    position: relative;
    width: 36px;
    height: 110px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: ns-resize;
  }

  .groove {
    position: absolute;
    width: 10px;
    height: 100%;
    border-radius: 6px;
    background: #e8e0d4;
    box-shadow:
      inset 3px 3px 7px rgba(150,130,100,0.35),
      inset -2px -2px 5px rgba(255,250,240,0.8);
    overflow: hidden;
  }

  .fill {
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    border-radius: 6px;
    background: linear-gradient(to top, rgba(170,140,90,0.55), rgba(200,175,130,0.3));
    transition: height 0.04s;
  }

  .thumb {
    position: absolute;
    width: 30px;
    height: 22px;
    border-radius: 8px;
    background: radial-gradient(ellipse at 40% 30%, #f5efe6, #ddd4c2);
    box-shadow:
      3px 3px 8px rgba(150,130,100,0.45),
      -2px -2px 7px rgba(255,252,242,0.9);
    z-index: 2;
    cursor: grab;
    transition: box-shadow 0.12s;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .thumb::after {
    content: '';
    width: 14px;
    height: 1.5px;
    border-radius: 1px;
    background: rgba(140,115,80,0.4);
  }

  .thumb.pressed {
    cursor: grabbing;
    box-shadow:
      1px 1px 4px rgba(150,130,100,0.5),
      -1px -1px 3px rgba(255,252,242,0.8),
      inset 2px 2px 5px rgba(140,120,90,0.2);
  }

  .val {
    font-family: 'DM Sans', sans-serif;
    font-size: 0.58rem;
    font-weight: 200;
    color: rgba(150, 120, 80, 0.65);
    letter-spacing: 0.08em;
  }
</style>
