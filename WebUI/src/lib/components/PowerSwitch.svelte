<script lang="ts">
	import { toggles } from '../../state/store';
	import { setFlag } from '$lib/stores/amp';
	import { tooltip } from '$lib/actions/tooltip';
	import { menu, parseFlag } from '$lib/actions/menu';
	import Led from './Led.svelte';

	// POWER = !bypass (spec §6, §7.3). Same parameter as the toolbar power icon.
	const bypass = toggles.bypass;
	const on = $derived(!$bypass);
</script>

<div class="power">
	<span class="on-led"><Led on={on} size={16} /></span>
	<span class="caption on-caption">ON</span>
	<button
		type="button"
		class="rocker"
		class:on
		role="switch"
		aria-checked={on}
		aria-label="POWER"
		onclick={() => setFlag('bypass', on)}
		use:tooltip={{ text: `POWER ${on ? 'On' : 'Off'}` }}
		use:menu={{
			param: 'bypass',
			reset: () => setFlag('bypass', false),
			copy: () => (on ? 'on' : 'off'),
			paste: (t) => {
				const v = parseFlag(t);
				if (v === null) return false;
				setFlag('bypass', !v);
				return true;
			}
		}}
	>
		<span class="mark top" aria-hidden="true">I</span>
		<span class="mark bottom" aria-hidden="true">O</span>
	</button>
	<span class="caption power-caption">POWER</span>
</div>

<style>
	/* Container is the rocker box (84 × 106), centred at 2282, 366. */
	.power {
		position: relative;
		width: calc(84 * var(--u));
		height: calc(106 * var(--u));
	}
	.on-led,
	.on-caption,
	.power-caption {
		position: absolute;
		transform: translate(-50%, -50%);
		pointer-events: none;
	}
	/* Offsets from the rocker's top-left (2240, 313). */
	.on-led {
		left: calc(20 * var(--u));
		top: calc(-23 * var(--u));
		line-height: 0;
	}
	.on-caption {
		left: calc(57 * var(--u));
		top: calc(-25 * var(--u));
	}
	.power-caption {
		left: calc(45 * var(--u));
		top: calc(127 * var(--u));
	}
	.rocker {
		all: unset;
		position: absolute;
		inset: 0;
		box-sizing: border-box;
		border-radius: calc(10 * var(--u));
		border: calc(3 * var(--u)) solid var(--groove);
		cursor: pointer;
		overflow: hidden;
		/* Off: bottom half pressed in. */
		background: linear-gradient(to bottom, #2b2d31 0%, #1a1b1e 48%, #070708 52%, #121315 100%);
		box-shadow:
			0 calc(4 * var(--u)) calc(10 * var(--u)) rgba(0, 0, 0, 0.6),
			inset 0 calc(1 * var(--u)) 0 rgba(255, 255, 255, 0.08);
	}
	.rocker.on {
		background: linear-gradient(to bottom, #070708 0%, #121315 48%, #1a1b1e 52%, #2b2d31 100%);
	}
	.rocker:focus-visible {
		outline: 2px solid var(--accent-blue);
		outline-offset: 3px;
	}
	.mark {
		position: absolute;
		left: 50%;
		transform: translateX(-50%);
		font-family: var(--font-ui);
		font-weight: 600;
		font-size: calc(24 * var(--u));
		color: var(--ink-soft);
		opacity: 0.45;
		line-height: 1;
	}
	.mark.top {
		top: calc(14 * var(--u));
	}
	.mark.bottom {
		bottom: calc(14 * var(--u));
	}
	.rocker.on .mark.top,
	.rocker:not(.on) .mark.bottom {
		opacity: 1;
		color: var(--ink-strong);
	}
</style>
