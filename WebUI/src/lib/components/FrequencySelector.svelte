<script lang="ts">
	import {
		midFreqPos,
		setMidFreqPos,
		commitMidFreqPos,
		beginGesture,
		endGesture,
		MID_FREQ_HZ,
		MID_FREQ_DEFAULT_POS
	} from '$lib/stores/amp';
	import { drag, wheel } from '$lib/actions/drag';
	import { tooltip } from '$lib/actions/tooltip';
	import { menu, parseNumber } from '$lib/actions/menu';

	const DRAG_PX_PER_DETENT = 40;
	const DETENTS = [1, 2, 3, 4, 5];

	const angle = $derived(-135 + ($midFreqPos - 1) * 67.5);
	const valueText = $derived(`${$midFreqPos} · ${MID_FREQ_HZ[$midFreqPos - 1]}`);

	let dragging = $state(false);
	let startPos = 0;

	const clampPos = (p: number) => Math.min(5, Math.max(1, Math.round(p)));
	const commit = (p: number) => commitMidFreqPos(clampPos(p));
	const reset = () => commit(MID_FREQ_DEFAULT_POS);

	function onKeydown(e: KeyboardEvent) {
		if (e.key === 'ArrowUp' || e.key === 'ArrowRight') commit($midFreqPos + 1);
		else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') commit($midFreqPos - 1);
		else if (e.key === 'Home') commit(1);
		else if (e.key === 'End') commit(5);
		else return;
		e.preventDefault();
	}
</script>

<div
	class="selector"
	class:dragging
	style:--angle="{angle}deg"
	role="slider"
	tabindex="0"
	aria-label="FREQUENCY"
	aria-valuemin={1}
	aria-valuemax={5}
	aria-valuenow={$midFreqPos}
	aria-valuetext={valueText}
	ondblclick={reset}
	onkeydown={onKeydown}
	use:drag={{
		onStart: () => {
			dragging = true;
			startPos = $midFreqPos;
			beginGesture('midFreq');
		},
		onDrag: (dy) => setMidFreqPos(clampPos(startPos + dy / DRAG_PX_PER_DETENT)),
		onEnd: () => {
			dragging = false;
			endGesture('midFreq');
		}
	}}
	use:wheel={(dir) => commit($midFreqPos + dir)}
	use:tooltip={{ text: `FREQUENCY ${valueText}`, active: dragging }}
	use:menu={{
		param: 'midFreq',
		reset,
		copy: () => String($midFreqPos),
		paste: (t) => {
			const n = parseNumber(t);
			if (n === null || n < 1 || n > 5) return false;
			commit(n);
			return true;
		}
	}}
>
	{#each DETENTS as p (p)}
		<span class="detent" style:--a="{-135 + (p - 1) * 67.5}deg" aria-hidden="true"></span>
	{/each}
	<span class="body" aria-hidden="true">
		<span class="pointer"></span>
	</span>
</div>

<style>
	.selector {
		position: relative;
		width: calc(76 * var(--u));
		height: calc(76 * var(--u));
		border-radius: 50%;
		box-sizing: border-box;
		border: calc(3 * var(--u)) solid var(--groove);
		box-shadow: 0 calc(4 * var(--u)) calc(8 * var(--u)) rgba(0, 0, 0, 0.55);
		cursor: ns-resize;
		touch-action: none;
		transition: border-color 120ms;
	}
	.selector:hover,
	.selector.dragging {
		border-color: var(--button-edge);
	}
	.body {
		position: absolute;
		inset: 0;
		border-radius: 50%;
		background: radial-gradient(
			circle at 40% 35%,
			var(--knob-face-hi) 0%,
			#1d1f22 55%,
			var(--knob-face-lo) 100%
		);
		transform: rotate(var(--angle));
		transition: transform 80ms ease-out;
	}
	.pointer {
		position: absolute;
		left: 50%;
		top: 4%;
		width: calc(4 * var(--u));
		height: 31%;
		transform: translateX(-50%);
		background: var(--ink-strong);
		border-radius: calc(2 * var(--u));
	}
	/* Detent dots just outside the ring. */
	.detent {
		position: absolute;
		left: 50%;
		top: 50%;
		width: calc(4 * var(--u));
		height: calc(4 * var(--u));
		margin: calc(-2 * var(--u));
		border-radius: 50%;
		background: var(--ink-soft);
		transform: rotate(var(--a)) translateY(calc(-46 * var(--u)));
	}
</style>
