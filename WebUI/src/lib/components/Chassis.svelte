<script lang="ts">
	import type { Snippet } from 'svelte';

	let { children }: { children?: Snippet } = $props();

	const HOLES = [
		{ x: 50, y: 100 },
		{ x: 2512, y: 100 },
		{ x: 45, y: 500 },
		{ x: 2512, y: 500 }
	];
</script>

<!-- Decoration only. All positions in plugin reference px. -->
<div class="shell" aria-hidden="true">
	<div class="top-strip"></div>
	<div class="grip left"></div>
	<div class="grip right"></div>
	{#each HOLES as h (h.x + ',' + h.y)}
		<span class="at hole" style:--x={h.x} style:--y={h.y}></span>
	{/each}
	<span class="at screw" style:--x={1283} style:--y={160} style:--d={28}></span>
	<span class="at screw" style:--x={1280} style:--y={497} style:--d={24}></span>
	<span class="at wordmark" style:--x={1830} style:--y={165}>EPIC-AMP-1</span>
</div>

{@render children?.()}

<style>
	/* Spans the whole plugin so children use plugin coordinates; the body
	   itself starts below the 60 px toolbar. */
	.shell {
		position: absolute;
		inset: 0;
		pointer-events: none;
	}
	.shell::before {
		content: '';
		position: absolute;
		left: 0;
		right: 0;
		top: calc(60 * var(--u));
		bottom: 0;
		background: linear-gradient(to bottom, #44484e 0%, var(--shell-mid) 30%, var(--shell-dark) 100%);
	}
	.top-strip {
		position: absolute;
		left: 0;
		right: 0;
		top: calc(60 * var(--u));
		height: calc(172 * var(--u));
		background:
			repeating-linear-gradient(
				to bottom,
				rgba(255, 255, 255, 0.025) 0 1px,
				transparent 1px 3px
			),
			linear-gradient(to bottom, #474b51, var(--shell-mid));
	}
	.grip {
		position: absolute;
		top: calc(115 * var(--u));
		width: calc(124 * var(--u));
		height: calc(380 * var(--u));
		border-radius: calc(20 * var(--u));
		background: linear-gradient(to right, #1b1d20, var(--shell-dark) 40%, #33373c 60%, #1b1d20);
		box-shadow:
			0 calc(6 * var(--u)) calc(14 * var(--u)) rgba(0, 0, 0, 0.6),
			inset 0 calc(1 * var(--u)) 0 rgba(255, 255, 255, 0.08);
	}
	.grip.left {
		left: calc(74 * var(--u));
	}
	.grip.right {
		left: calc(2366 * var(--u));
	}
	.hole {
		width: calc(36 * var(--u));
		height: calc(36 * var(--u));
		border-radius: 50%;
		background: radial-gradient(circle at 50% 40%, #050506 55%, #2c2f33 100%);
		box-shadow: 0 calc(1 * var(--u)) 0 rgba(255, 255, 255, 0.08);
	}
	.screw {
		width: calc(var(--d) * var(--u));
		height: calc(var(--d) * var(--u));
		border-radius: 50%;
		background:
			linear-gradient(45deg, transparent 45%, #1b1c1f 45% 55%, transparent 55%),
			radial-gradient(circle at 40% 35%, #b9bdc2, #5d6167 70%);
		box-shadow: 0 calc(1 * var(--u)) calc(2 * var(--u)) rgba(0, 0, 0, 0.6);
	}
	.wordmark {
		font-family: var(--font-wordmark);
		font-style: italic;
		font-size: calc(60 * var(--u));
		line-height: 1;
		color: var(--ink-strong);
		text-shadow: 0 calc(2 * var(--u)) calc(3 * var(--u)) rgba(0, 0, 0, 0.6);
		white-space: nowrap;
	}
</style>
