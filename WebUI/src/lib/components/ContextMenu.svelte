<script lang="ts">
	import { menu, type MenuItem } from '$lib/stores/overlay';

	let el = $state<HTMLDivElement>();

	const close = () => menu.set(null);

	function run(item: MenuItem) {
		close();
		void item.action();
	}

	$effect(() => {
		if ($menu) el?.querySelector<HTMLButtonElement>('button')?.focus();
	});

	function onWindowPointerDown(e: PointerEvent) {
		if ($menu && el && !el.contains(e.target as Node)) close();
	}

	function onWindowKeydown(e: KeyboardEvent) {
		if ($menu && e.key === 'Escape') close();
	}
</script>

<svelte:window onpointerdown={onWindowPointerDown} onkeydown={onWindowKeydown} onblur={close} />

{#if $menu}
	<div bind:this={el} class="menu" role="menu" style:left="{$menu.x}px" style:top="{$menu.y}px">
		{#each $menu.items as item (item.label)}
			<button type="button" role="menuitem" onclick={() => run(item)}>{item.label}</button>
		{/each}
	</div>
{/if}

<style>
	.menu {
		position: fixed;
		z-index: 1001;
		min-width: 140px;
		padding: 4px 0;
		border-radius: 6px;
		background: #1b1d20;
		border: 1px solid #3b3f44;
		box-shadow: 0 6px 18px rgba(0, 0, 0, 0.6);
	}
	button {
		all: unset;
		display: block;
		width: 100%;
		box-sizing: border-box;
		padding: 6px 12px;
		font-family: var(--font-ui);
		font-size: 13px;
		color: var(--ink-strong);
		cursor: pointer;
	}
	button:hover,
	button:focus-visible {
		background: var(--accent-blue);
		color: #ffffff;
	}
</style>
