<script lang="ts">
	import { toggles } from '../../state/store';
	import { setFlag, getFlag, LABELS, type FlagKey } from '$lib/stores/amp';
	import { tooltip } from '$lib/actions/tooltip';
	import { menu, parseFlag } from '$lib/actions/menu';
	import Led from './Led.svelte';

	let {
		key,
		caption,
		captionPlacement = 'above'
	}: { key: Exclude<FlagKey, 'bypass'>; caption: string; captionPlacement?: 'above' | 'below' } =
		$props();

	// svelte-ignore state_referenced_locally
	const flag = toggles[key];
	// svelte-ignore state_referenced_locally
	const label = LABELS[key];
</script>

<div class="toggle">
	<div class="legend {captionPlacement}">
		<Led on={$flag} />
		<span class="caption">{caption}</span>
	</div>
	<button
		type="button"
		class="face"
		class:on={$flag}
		role="switch"
		aria-checked={$flag}
		aria-label={label}
		onclick={() => setFlag(key, !$flag)}
		use:tooltip={{ text: `${label} ${$flag ? 'On' : 'Off'}` }}
		use:menu={{
			param: key,
			reset: () => setFlag(key, false),
			copy: () => (getFlag(key) ? 'on' : 'off'),
			paste: (t) => {
				const v = parseFlag(t);
				if (v === null) return false;
				setFlag(key, v);
				return true;
			}
		}}
	></button>
</div>

<style>
	.toggle {
		position: relative;
		width: calc(56 * var(--u));
		height: calc(30 * var(--u));
	}
	/* LED sits 43 ref px left of the button centre; the row is ~40 px above
	   or ~37 px below the button (spec §2.4). */
	.legend {
		position: absolute;
		left: calc(-22 * var(--u));
		display: flex;
		align-items: center;
		gap: calc(14 * var(--u));
		pointer-events: none;
		transform: translateY(-50%);
	}
	.legend.above {
		top: calc(-27 * var(--u));
	}
	.legend.below {
		top: calc(51 * var(--u));
	}
	.legend .caption {
		font-size: calc(20 * var(--u));
	}
	.face {
		all: unset;
		position: absolute;
		inset: 0;
		box-sizing: border-box;
		border: calc(2 * var(--u)) solid var(--button-edge);
		border-radius: calc(4 * var(--u));
		background: linear-gradient(to bottom, var(--button-face-hi), var(--button-face-lo));
		box-shadow: 0 calc(2 * var(--u)) calc(4 * var(--u)) rgba(0, 0, 0, 0.5);
		cursor: pointer;
	}
	.face.on {
		background: linear-gradient(to bottom, var(--button-face-lo), var(--button-face-hi));
	}
	.face:active {
		box-shadow: inset 0 2px 3px rgba(0, 0, 0, 0.6);
	}
	.face:focus-visible {
		outline: 2px solid var(--accent-blue);
		outline-offset: 3px;
	}
</style>
