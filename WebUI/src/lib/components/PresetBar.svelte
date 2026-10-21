<script lang="ts">
	import { derived, readable } from 'svelte/store';
	import { applyPreset, presetActive } from '$lib/stores/amp';
	import { tooltip } from '$lib/actions/tooltip';
	import { PRESETS } from '$lib/presets';

	// One store per preset: lit while every value still equals the preset.
	const active = derived(
		PRESETS.map((p) => (p.values ? presetActive(p.values) : readable(false))),
		(flags) => flags
	);
</script>

<div class="at presets" style:--x={1137} style:--y={30} role="group" aria-label="Presets">
	{#each PRESETS as p, i (p.name)}
		{@const values = p.values}
		<button
			type="button"
			class={['preset', { selected: $active[i] }]}
			aria-pressed={$active[i]}
			aria-disabled={!values}
			use:tooltip={{ text: p.note }}
			onclick={() => values && applyPreset(values)}>{p.name}</button
		>
	{/each}
</div>

<style>
	.presets {
		width: calc(320 * var(--u));
		display: flex;
		justify-content: space-between;
		align-items: center;
	}
	.preset {
		margin: 0;
		padding: 0 0 calc(2 * var(--u));
		border: none;
		border-bottom: calc(2 * var(--u)) solid transparent;
		background: none;
		cursor: pointer;
		color: var(--toolbar-text);
		font-family: var(--font-ui);
		font-size: calc(26 * var(--u));
		font-weight: 400;
		line-height: 1;
		white-space: nowrap;
	}
	.preset:focus-visible {
		outline: 2px solid var(--accent-blue);
		outline-offset: 3px;
	}
	.preset:not([aria-disabled='true']):hover {
		color: #ffffff;
		border-bottom-color: currentColor;
	}
	.preset.selected {
		color: var(--accent-blue);
	}
	.preset[aria-disabled='true'] {
		opacity: 0.35;
		cursor: default;
	}
</style>
