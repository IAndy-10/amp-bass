<script lang="ts">
	import { sliders, toggles, combos } from '../../state/store';
	import {
		setSliderValue,
		sliderDragStarted,
		sliderDragEnded,
		setToggleValue,
		setComboIndex
	} from '../../bridge/bridge';
	import { fxRackOpen } from '$lib/stores/overlay';
	import { fromStore } from 'svelte/store';
	import type { SliderParamId } from '../../types/parameters';

	// FX Rack: the parameters that have no faceplate control. Plain sliders on the
	// normalised value; ranges, defaults and text follow ParameterLayout.h.
	// Not in the undo history (like the presets' FX values).
	interface Fader {
		id: SliderParamId;
		label: string;
		/** Normalised default (double-click resets). */
		def: number;
		text: (n: number) => string;
	}

	const db = (v: number) => `${v.toFixed(1)} dB`;
	const FADERS: Record<string, Fader> = {
		inputGain: { id: 'inputGain', label: 'Input', def: 0.5, text: (n) => db(-20 + 40 * n) },
		gateThreshold: { id: 'gateThreshold', label: 'Threshold', def: 32 / 120, text: (n) => db(-120 + 120 * n) },
		gateRange: { id: 'gateRange', label: 'Range', def: 0.5, text: (n) => (n >= 1 ? 'Mute' : db(100 * n)) },
		tightFreq: {
			id: 'tightFreq',
			label: 'Frequency',
			def: Math.log(50 / 30) / Math.log(100),
			text: (n) => `${(30 * Math.pow(100, n)).toFixed(0)} Hz`
		},
		smoothFreq: {
			id: 'smoothFreq',
			label: 'Frequency',
			def: Math.log(7) / Math.log(35 / 3),
			text: (n) => `${(3 * Math.pow(35 / 3, n)).toFixed(1)} kHz`
		},
		powerSoak: { id: 'powerSoak', label: 'Power Soak', def: 0.9, text: (n) => db(-40 + 40 * n) },
		dryWet: { id: 'dryWet', label: 'Dry/Wet', def: 1, text: (n) => `${Math.round(100 * n)} % wet` }
	};

	const SECTIONS: { title: string; switch?: [SwitchId, string][]; mode?: 'tight' | 'smooth'; faders: Fader[] }[] = [
		{ title: 'Input', faders: [FADERS.inputGain] },
		{ title: 'Noise Gate', switch: [['gate', 'On']], faders: [FADERS.gateThreshold, FADERS.gateRange] },
		{ title: 'Tight', mode: 'tight', faders: [FADERS.tightFreq] },
		{ title: 'Smooth', mode: 'smooth', faders: [FADERS.smoothFreq] },
		{
			title: 'Output',
			switch: [
				['cabinet', 'Cabinet'],
				['horn', 'Horn']
			],
			faders: [FADERS.powerSoak, FADERS.dryWet]
		}
	];
	const MODES = ['Off', 'Pre', 'Post'];

	type SwitchId = 'gate' | 'cabinet' | 'horn';
	const on = { gate: fromStore(toggles.gate), cabinet: fromStore(toggles.cabinet), horn: fromStore(toggles.horn) };
	const mode = { tight: fromStore(combos.tight), smooth: fromStore(combos.smooth) };
	const value = Object.fromEntries(Object.values(FADERS).map((f) => [f.id, fromStore(sliders[f.id])]));

	let dragging: SliderParamId | null = null;

	function begin(id: SliderParamId) {
		dragging = id;
		sliderDragStarted(id);
	}
	function end(id: SliderParamId) {
		if (dragging !== id) return;
		dragging = null;
		sliderDragEnded(id);
	}
	function reset(f: Fader) {
		sliderDragStarted(f.id);
		setSliderValue(f.id, f.def);
		sliderDragEnded(f.id);
	}
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && fxRackOpen.set(false)} />

<section class="rack" aria-label="FX Rack">
	{#each SECTIONS as s (s.title)}
		<div class="unit">
			<h2>{s.title}</h2>

			{#if s.switch}
				<div class="row buttons">
					{#each s.switch as [id, label] (id)}
						<button
							type="button"
							class:on={on[id].current}
							role="switch"
							aria-checked={on[id].current}
							aria-label="{s.title} {label}"
							onclick={() => setToggleValue(id, !on[id].current)}>{label}</button
						>
					{/each}
				</div>
			{/if}

			{#if s.mode}
				{@const id = s.mode}
				<div class="row buttons" role="radiogroup" aria-label={s.title}>
					{#each MODES as m, i (m)}
						<button
							type="button"
							class:on={mode[id].current === i}
							role="radio"
							aria-checked={mode[id].current === i}
							onclick={() => setComboIndex(id, i)}>{m}</button
						>
					{/each}
				</div>
			{/if}

			{#each s.faders as f (f.id)}
				<label class="row fader">
					<span class="name">{f.label}</span>
					<span class="value">{f.text(value[f.id].current)}</span>
					<input
						type="range"
						min="0"
						max="1"
						step="0.001"
						value={value[f.id].current}
						title="Double-click to reset"
						onpointerdown={() => begin(f.id)}
						onpointerup={() => end(f.id)}
						onlostpointercapture={() => end(f.id)}
						oninput={(e) => setSliderValue(f.id, Number(e.currentTarget.value))}
						ondblclick={() => reset(f)}
					/>
				</label>
			{/each}
		</div>
	{/each}
</section>

<style>
	.rack {
		position: absolute;
		left: 0;
		top: calc(60 * var(--u));
		width: 100%;
		height: calc(498 * var(--u));
		box-sizing: border-box;
		z-index: 10;
		display: grid;
		grid-template-columns: repeat(5, 1fr);
		gap: calc(24 * var(--u));
		padding: calc(32 * var(--u)) calc(48 * var(--u));
		background: var(--shell-dark);
		font-family: var(--font-ui);
		font-size: calc(24 * var(--u));
		color: var(--ink-soft);
	}
	.unit {
		display: flex;
		flex-direction: column;
		gap: calc(22 * var(--u));
		padding: calc(20 * var(--u)) calc(24 * var(--u));
		border-radius: calc(10 * var(--u));
		background: var(--faceplate-lo);
	}
	h2 {
		margin: 0;
		font-size: calc(28 * var(--u));
		font-weight: 600;
		color: var(--ink-strong);
	}
	.buttons {
		display: flex;
		gap: calc(10 * var(--u));
	}
	button {
		flex: 1;
		margin: 0;
		padding: calc(10 * var(--u)) 0;
		border: 1px solid var(--shell-mid);
		border-radius: calc(6 * var(--u));
		background: var(--button-face-lo);
		color: var(--ink-soft);
		font: inherit;
		cursor: pointer;
	}
	button.on {
		background: var(--accent-blue);
		border-color: var(--accent-blue);
		color: #ffffff;
	}
	button:focus-visible,
	input:focus-visible {
		outline: 2px solid var(--accent-blue);
		outline-offset: 2px;
	}
	.fader {
		display: grid;
		grid-template-columns: 1fr auto;
		row-gap: calc(8 * var(--u));
	}
	.value {
		color: var(--ink-strong);
		font-variant-numeric: tabular-nums;
	}
	input {
		grid-column: 1 / -1;
		width: 100%;
		margin: 0;
		accent-color: var(--accent-blue);
		cursor: pointer;
	}
</style>
