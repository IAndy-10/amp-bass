<script lang="ts">
	import {
		numeric,
		setNumeric,
		commitNumeric,
		beginGesture,
		endGesture,
		getNumeric,
		rangeOf,
		formatValue,
		snap,
		DEFAULTS,
		LABELS,
		type KnobKey
	} from '$lib/stores/amp';
	import { drag, wheel } from '$lib/actions/drag';
	import { tooltip } from '$lib/actions/tooltip';
	import { menu, parseNumber } from '$lib/actions/menu';

	let { key, size = 'normal' }: { key: KnobKey; size?: 'normal' | 'large' } = $props();

	const DRAG_PX_FULL_RANGE = 200;

	// The key never changes for a mounted knob.
	// svelte-ignore state_referenced_locally
	const value = numeric(key);
	// svelte-ignore state_referenced_locally
	const { start: min, end: max } = rangeOf(key);
	// svelte-ignore state_referenced_locally
	const label = LABELS[key];

	const angle = $derived(-135 + (($value - min) / (max - min)) * 270);
	const valueText = $derived(formatValue(key, $value));

	let dragging = $state(false);
	let startValue = 0;

	const reset = () => commitNumeric(key, DEFAULTS[key]);

	function onKeydown(e: KeyboardEvent) {
		const step = e.shiftKey ? 1 : 0.1;
		const v = getNumeric(key);
		let next: number;
		switch (e.key) {
			case 'ArrowUp':
			case 'ArrowRight':
				next = v + step;
				break;
			case 'ArrowDown':
			case 'ArrowLeft':
				next = v - step;
				break;
			case 'Home':
				next = min;
				break;
			case 'End':
				next = max;
				break;
			default:
				return;
		}
		e.preventDefault();
		commitNumeric(key, next);
	}
</script>

<div
	class="knob {size}"
	class:dragging
	style:--angle="{angle}deg"
	role="slider"
	tabindex="0"
	aria-label={label}
	aria-valuemin={min}
	aria-valuemax={max}
	aria-valuenow={snap($value)}
	aria-valuetext={valueText}
	ondblclick={reset}
	onkeydown={onKeydown}
	use:drag={{
		onStart: () => {
			dragging = true;
			startValue = getNumeric(key);
			beginGesture(key);
		},
		onDrag: (dy) => setNumeric(key, startValue + (dy * (max - min)) / DRAG_PX_FULL_RANGE),
		onEnd: () => {
			dragging = false;
			endGesture(key);
		}
	}}
	use:wheel={(dir, shift) => commitNumeric(key, getNumeric(key) + dir * (shift ? 0.1 : 0.5))}
	use:tooltip={{ text: `${label} ${valueText}`, active: dragging }}
	use:menu={{
		param: key,
		reset,
		copy: () => formatValue(key, getNumeric(key)),
		paste: (t) => {
			const n = parseNumber(t);
			if (n === null) return false;
			commitNumeric(key, n);
			return true;
		}
	}}
>
	<span class="tick" aria-hidden="true"></span>
	<span class="body" aria-hidden="true">
		<span class="pointer"></span>
	</span>
</div>

<style>
	.knob {
		--d: 76;
		position: relative;
		width: calc(var(--d) * var(--u));
		height: calc(var(--d) * var(--u));
		border-radius: 50%;
		box-sizing: border-box;
		border: calc(3 * var(--u)) solid var(--groove);
		box-shadow:
			0 calc(4 * var(--u)) calc(8 * var(--u)) rgba(0, 0, 0, 0.55),
			0 0 0 calc(1 * var(--u)) rgba(255, 255, 255, 0.06);
		cursor: ns-resize;
		touch-action: none;
		transition:
			border-color 120ms,
			box-shadow 120ms;
	}
	.knob.large {
		--d: 96;
	}
	.knob:hover {
		border-color: var(--button-edge);
	}
	.knob.dragging {
		border-color: var(--button-edge);
		box-shadow:
			0 calc(4 * var(--u)) calc(8 * var(--u)) rgba(0, 0, 0, 0.55),
			0 0 calc(10 * var(--u)) calc(2 * var(--u)) rgba(62, 166, 230, 0.45);
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
	}
	/* Pointer: from 30 % to 92 % of the radius, 4 px wide. */
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
	.tick {
		position: absolute;
		left: 50%;
		top: calc(-9 * var(--u));
		width: calc(2 * var(--u));
		height: calc(5 * var(--u));
		transform: translateX(-50%);
		background: var(--ink-soft);
	}
</style>
