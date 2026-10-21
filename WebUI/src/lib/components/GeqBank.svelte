<script lang="ts">
	import { toggles } from '../../state/store';
	import type { GeqKey } from '$lib/stores/amp';
	import GeqSlider from './GeqSlider.svelte';

	// Positions in plugin reference px.
	const TRACK_Y = 357;
	const BANDS: { key: GeqKey; caption: string; x: number }[] = [
		{ key: 'geq33', caption: '33Hz', x: 1614 },
		{ key: 'geq80', caption: '80', x: 1673 },
		{ key: 'geq150', caption: '150', x: 1732 },
		{ key: 'geq300', caption: '300', x: 1791 },
		{ key: 'geq600', caption: '600', x: 1850 },
		{ key: 'geq900', caption: '900', x: 1909 },
		{ key: 'geq2k', caption: '2k', x: 1968 },
		{ key: 'geq5k', caption: '5k', x: 2027 },
		{ key: 'geq8k', caption: '9k', x: 2086 } // ID frozen as geq8k, printed 9k
	];
	const LEVEL_X = 2145;

	const geqOn = toggles.geqOn;
	const disabled = $derived(!$geqOn);
</script>

<div class="field" aria-hidden="true"></div>

<div class="bank" class:disabled role="group" aria-label="Graphic EQ">
	{#each BANDS as b (b.key)}
		<span class="at band-caption" style:--x={b.x} style:--y={260} aria-hidden="true">{b.caption}</span>
		<div class="at" style:--x={b.x} style:--y={TRACK_Y}>
			<GeqSlider key={b.key} {disabled} />
		</div>
	{/each}
	<div class="at" style:--x={LEVEL_X} style:--y={TRACK_Y}>
		<GeqSlider key="geqLevel" variant="level" {disabled} />
	</div>

	<span class="at numeral" style:--x={1563} style:--y={285} aria-hidden="true">+12dB</span>
	<span class="at numeral" style:--x={1585} style:--y={357} aria-hidden="true">0</span>
	<span class="at numeral" style:--x={1565} style:--y={430} aria-hidden="true">-12dB</span>

	<span class="at numeral" style:--x={2195} style:--y={285} aria-hidden="true">+8dB</span>
	<span class="at numeral level-caption" style:--x={2203} style:--y={357} aria-hidden="true">LEVEL</span>
	<span class="at numeral" style:--x={2195} style:--y={430} aria-hidden="true">-10dB</span>
</div>

<style>
	/* Recessed field: box 1590,262 to 2165,446. */
	.field {
		position: absolute;
		left: calc(1590 * var(--u));
		top: calc(262 * var(--u));
		width: calc(575 * var(--u));
		height: calc(184 * var(--u));
		background: var(--field-inset);
		border-radius: calc(6 * var(--u));
		box-shadow:
			inset 0 calc(3 * var(--u)) calc(6 * var(--u)) rgba(0, 0, 0, 0.55),
			0 calc(1 * var(--u)) 0 rgba(255, 255, 255, 0.06);
	}
	.bank {
		display: contents;
	}
	.band-caption {
		font-family: var(--font-ui);
		font-size: calc(22 * var(--u));
		font-weight: 600;
		font-style: italic;
		color: var(--ink-strong);
		text-shadow: 0 1px 0 rgba(0, 0, 0, 0.7);
		white-space: nowrap;
		line-height: 1;
		transition: opacity 120ms;
	}
	.numeral {
		font-size: calc(20 * var(--u));
	}
	.level-caption {
		font-weight: 600;
		font-style: italic;
		color: var(--ink-strong);
	}
	.disabled .band-caption {
		opacity: 0.4;
	}
</style>
