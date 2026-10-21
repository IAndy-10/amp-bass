<script lang="ts">
	import Toolbar from './Toolbar.svelte';
	import Chassis from './Chassis.svelte';
	import Faceplate from './Faceplate.svelte';
	import Tooltip from './Tooltip.svelte';
	import ContextMenu from './ContextMenu.svelte';
	import FxRack from './FxRack.svelte';
	import { fxRackOpen } from '$lib/stores/overlay';
	import { undo, redo } from '$lib/stores/history';

	// Cmd/Ctrl+Z undo, Cmd/Ctrl+Shift+Z redo.
	function onKeydown(e: KeyboardEvent) {
		if (!(e.metaKey || e.ctrlKey) || e.key.toLowerCase() !== 'z') return;
		e.preventDefault();
		if (e.shiftKey) redo();
		else undo();
	}
</script>

<svelte:window onkeydown={onKeydown} />

<div class="stage">
	<div class="plugin">
		<Toolbar />
		<Chassis>
			<Faceplate />
		</Chassis>
		{#if $fxRackOpen}
			<FxRack />
		{/if}
	</div>
</div>

<Tooltip />
<ContextMenu />

<style>
	/* 2562 × 558 reference-pixel canvas, capped at 1800 px, scrolls below 960 px. */
	.stage {
		container-type: inline-size;
		width: 100%;
		max-width: 1800px;
		min-width: 960px;
		margin: 0 auto;
	}
	.plugin {
		--u: calc(100cqw / 2562);
		position: relative;
		width: 100cqw;
		height: calc(558 * var(--u));
		overflow: hidden;
		user-select: none;
		-webkit-user-select: none;
	}
</style>
