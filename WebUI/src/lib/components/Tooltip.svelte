<script lang="ts">
	import { tooltip } from '$lib/stores/overlay';

	const visible = $derived(!!$tooltip.text && !!$tooltip.anchor);
	const left = $derived($tooltip.anchor ? $tooltip.anchor.left + $tooltip.anchor.width / 2 : 0);
	const top = $derived($tooltip.anchor ? $tooltip.anchor.top : 0);
</script>

{#if visible}
	<div class="tooltip" role="tooltip" style:left="{left}px" style:top="{top}px">
		{$tooltip.text}
	</div>
{/if}

<style>
	.tooltip {
		position: fixed;
		z-index: 1000;
		transform: translate(-50%, calc(-100% - 8px));
		padding: 4px 8px;
		border-radius: 4px;
		background: rgba(0, 0, 0, 0.88);
		color: var(--ink-strong);
		font-family: var(--font-ui);
		font-size: 12px;
		font-weight: 500;
		white-space: nowrap;
		pointer-events: none;
		box-shadow: 0 2px 6px rgba(0, 0, 0, 0.5);
	}
</style>
