<script lang="ts">
	let {
		on,
		color = 'green',
		size = 14
	}: { on: boolean; color?: 'green' | 'red'; size?: number } = $props();
</script>

<span class="led {color}" class:on style:--d={size} aria-hidden="true"></span>

<style>
	.led {
		display: block;
		width: calc(var(--d) * var(--u));
		height: calc(var(--d) * var(--u));
		border-radius: 50%;
		background: var(--led-off);
		box-shadow: inset 0 calc(1 * var(--u)) calc(2 * var(--u)) rgba(0, 0, 0, 0.6);
	}
	.led.green.on {
		background: radial-gradient(circle at 40% 35%, #d9ffa8, var(--led-on) 60%);
		box-shadow: 0 0 calc(8 * var(--u)) calc(2 * var(--u)) rgba(156, 240, 60, 0.55);
	}
	.led.red {
		background: var(--led-red-off);
	}
	.led.red.on {
		background: radial-gradient(circle at 40% 35%, #ffb3a8, var(--led-red) 60%);
		box-shadow: 0 0 calc(8 * var(--u)) calc(2 * var(--u)) rgba(229, 57, 43, 0.6);
	}
</style>
