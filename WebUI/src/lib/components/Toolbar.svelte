<script lang="ts">
	import { toggles } from '../../state/store';
	import { setFlag } from '$lib/stores/amp';
	import { undo, redo, canUndo, canRedo } from '$lib/stores/history';

	const bypass = toggles.bypass;
	const active = $derived(!$bypass);

	// Presets, Copy, Paste, Reset, FX Rack and About are visual stubs in this
	// build (spec §0, §4): they render and focus but do nothing.
	const PRESETS = [
		{ id: 'A', x: 1050 },
		{ id: 'B', x: 1112 },
		{ id: 'C', x: 1172 },
		{ id: 'D', x: 1232 }
	];
	const STUBS = [
		{ label: 'Copy', x: 1373 },
		{ label: 'Paste', x: 1506 },
		{ label: 'Reset', x: 1667 },
		{ label: 'FX Rack', x: 1848 }
	];
</script>

<div class="toolbar" role="toolbar" aria-label="Plugin toolbar">
	<button
		type="button"
		class="at icon power"
		class:active
		style:--x={18}
		style:--y={30}
		role="switch"
		aria-checked={active}
		aria-label="Plugin power"
		title={active ? 'Plugin active' : 'Plugin bypassed'}
		onclick={() => setFlag('bypass', active)}
	>
		<svg viewBox="0 0 24 24" aria-hidden="true">
			<path d="M12 3v9" />
			<path d="M6.6 6.6a8 8 0 1 0 10.8 0" />
		</svg>
	</button>

	<button
		type="button"
		class="at icon"
		style:--x={870}
		style:--y={30}
		aria-label="Undo"
		disabled={!$canUndo}
		onclick={undo}
	>
		<svg viewBox="0 0 24 24" aria-hidden="true">
			<path d="M9 14 4 9l5-5" />
			<path d="M4 9h10.5a5.5 5.5 0 0 1 0 11H11" />
		</svg>
	</button>
	<button
		type="button"
		class="at icon"
		style:--x={930}
		style:--y={30}
		aria-label="Redo"
		disabled={!$canRedo}
		onclick={redo}
	>
		<svg viewBox="0 0 24 24" aria-hidden="true">
			<path d="m15 14 5-5-5-5" />
			<path d="M20 9H9.5a5.5 5.5 0 0 0 0 11H13" />
		</svg>
	</button>

	<div role="radiogroup" aria-label="Presets" class="presets">
		{#each PRESETS as p (p.id)}
			<button
				type="button"
				class="at text preset"
				class:selected={p.id === 'A'}
				style:--x={p.x}
				style:--y={30}
				role="radio"
				aria-checked={p.id === 'A'}
				aria-disabled="true">{p.id}</button
			>
		{/each}
	</div>

	{#each STUBS as s (s.label)}
		<button type="button" class="at text" style:--x={s.x} style:--y={30} aria-disabled="true"
			>{s.label}</button
		>
	{/each}

	<button type="button" class="text about" aria-disabled="true">About</button>
</div>

<style>
	.toolbar {
		position: absolute;
		left: 0;
		top: 0;
		width: 100%;
		height: calc(60 * var(--u));
		background: var(--toolbar-bg);
	}
	.presets {
		display: contents;
	}
	/* No `all: unset` here: it would also reset the global .at positioning. */
	button {
		margin: 0;
		padding: 0;
		border: none;
		background: none;
		font: inherit;
		box-sizing: border-box;
		cursor: pointer;
		color: var(--toolbar-text);
	}
	button:focus-visible {
		outline: 2px solid var(--accent-blue);
		outline-offset: 3px;
	}
	.text {
		font-family: var(--font-ui);
		font-size: calc(28 * var(--u));
		font-weight: 400;
		line-height: 1;
		padding-bottom: calc(2 * var(--u));
		border-bottom: calc(2 * var(--u)) solid transparent;
		white-space: nowrap;
	}
	.text:hover {
		color: #ffffff;
		border-bottom-color: currentColor;
	}
	.preset.selected {
		color: var(--accent-blue);
	}
	.about {
		position: absolute;
		right: calc(10 * var(--u));
		top: 50%;
		transform: translateY(-50%);
	}
	.icon {
		width: calc(30 * var(--u));
		height: calc(30 * var(--u));
		display: grid;
		place-items: center;
	}
	.icon svg {
		width: 100%;
		height: 100%;
		fill: none;
		stroke: currentColor;
		stroke-width: 2;
		stroke-linecap: round;
		stroke-linejoin: round;
	}
	.icon:hover {
		color: #ffffff;
	}
	.icon:disabled {
		opacity: 0.35;
		cursor: default;
	}
	.power {
		color: #6b6f75;
	}
	.power.active {
		color: var(--accent-blue);
	}
</style>
