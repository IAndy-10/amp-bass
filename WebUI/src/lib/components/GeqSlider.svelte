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
		type GeqKey
	} from '$lib/stores/amp';
	import { drag, wheel } from '$lib/actions/drag';
	import { tooltip } from '$lib/actions/tooltip';
	import { menu, parseNumber } from '$lib/actions/menu';

	let {
		key,
		variant = 'band',
		disabled = false
	}: { key: GeqKey; variant?: 'band' | 'level'; disabled?: boolean } = $props();

	// svelte-ignore state_referenced_locally
	const value = numeric(key);
	// svelte-ignore state_referenced_locally
	const label = LABELS[key];

	// Read per render so the relay's real range wins once JUCE sends it.
	const range = $derived.by(() => {
		void $value;
		return rangeOf(key);
	});
	const min = $derived(range.start);
	const max = $derived(range.end);
	const fraction = $derived(Math.min(1, Math.max(0, ($value - min) / (max - min))));
	const valueText = $derived(formatValue(key, $value));
	// Fine lines every 3 dB.
	const lineCount = $derived(Math.round((max - min) / 3) + 1);

	let el: HTMLDivElement;
	let dragging = $state(false);
	let startValue = 0;
	let trackPx = 1;

	const reset = () => commitNumeric(key, DEFAULTS[key]);

	function onKeydown(e: KeyboardEvent) {
		const v = getNumeric(key);
		let next: number;
		switch (e.key) {
			case 'ArrowUp':
				next = v + 0.1;
				break;
			case 'ArrowDown':
				next = v - 0.1;
				break;
			case 'PageUp':
				next = v + 1;
				break;
			case 'PageDown':
				next = v - 1;
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
	bind:this={el}
	class="geq {variant}"
	class:disabled
	class:dragging
	style:--f={fraction}
	style:--lines={lineCount}
	role="slider"
	tabindex={disabled ? -1 : 0}
	aria-label={label}
	aria-orientation="vertical"
	aria-valuemin={min}
	aria-valuemax={max}
	aria-valuenow={snap($value)}
	aria-valuetext={valueText}
	aria-disabled={disabled}
	ondblclick={reset}
	onkeydown={onKeydown}
	use:drag={{
		disabled,
		onStart: () => {
			dragging = true;
			startValue = getNumeric(key);
			// Full span of the track = full range (spec §10).
			trackPx = el.getBoundingClientRect().height || 1;
			beginGesture(key);
		},
		onDrag: (dy) => setNumeric(key, startValue + (dy / trackPx) * (max - min)),
		onEnd: () => {
			dragging = false;
			endGesture(key);
		}
	}}
	use:wheel={(dir, shift) => {
		if (!disabled) commitNumeric(key, getNumeric(key) + dir * (shift ? 0.1 : 0.5));
	}}
	use:tooltip={{ text: `${label} ${valueText}`, active: dragging }}
	use:menu={{
		param: key,
		reset,
		copy: () => formatValue(key, getNumeric(key)),
		paste: (t) => {
			const n = parseNumber(t);
			if (n === null || disabled) return false;
			commitNumeric(key, n);
			return true;
		}
	}}
>
	<span class="groove" aria-hidden="true"></span>
	<span class="lines" aria-hidden="true"></span>
	<span class="cap" aria-hidden="true"></span>
</div>

<style>
	/* Box = the 150-ref-px track, centred on the track centre. */
	.geq {
		position: relative;
		width: calc(44 * var(--u));
		height: calc(150 * var(--u));
		cursor: ns-resize;
		touch-action: none;
	}
	.groove {
		position: absolute;
		left: 50%;
		top: calc(-4 * var(--u));
		bottom: calc(-4 * var(--u));
		width: calc(6 * var(--u));
		transform: translateX(-50%);
		background: var(--groove);
		border-radius: calc(3 * var(--u));
		box-shadow: inset 0 0 calc(2 * var(--u)) rgba(0, 0, 0, 0.9);
	}
	.lines {
		position: absolute;
		left: 18%;
		right: 18%;
		top: 0;
		height: calc(150 * var(--u) + 1px);
		background: repeating-linear-gradient(
			to bottom,
			rgba(205, 208, 212, 0.35) 0 1px,
			transparent 1px calc(150 * var(--u) / (var(--lines) - 1))
		);
		pointer-events: none;
	}
	.cap {
		position: absolute;
		left: 50%;
		top: calc((1 - var(--f)) * 150 * var(--u));
		width: calc(24 * var(--u));
		height: calc(16 * var(--u));
		transform: translate(-50%, -50%);
		border-radius: calc(2 * var(--u));
		background: linear-gradient(to bottom, var(--thumb-hi), var(--thumb-lo));
		box-shadow:
			0 calc(2 * var(--u)) calc(4 * var(--u)) rgba(0, 0, 0, 0.6),
			inset 0 calc(-1 * var(--u)) 0 rgba(0, 0, 0, 0.3);
	}
	.cap::after {
		content: '';
		position: absolute;
		left: 15%;
		right: 15%;
		top: 50%;
		height: 1px;
		background: rgba(0, 0, 0, 0.55);
	}
	.geq:hover .cap,
	.geq.dragging .cap {
		background: linear-gradient(to bottom, #ffffff, var(--thumb-lo));
	}
	.geq.disabled {
		opacity: 0.4;
		pointer-events: none;
		cursor: default;
	}
	.geq.disabled .cap {
		background: #7a7d82;
	}
</style>
