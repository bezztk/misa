<script lang="ts">
    import { getTasks } from "../../../lib/api/tasks";
    import type { Task } from "../../../lib/models/task";

    let tasks: Task[] = $state([]);
    let error = $state("");

    async function loadTasks() {
        try {
            error = "";
            tasks = await getTasks();
        } catch (err) {
            error = String(err);
        }
    }
</script>

<button onclick={loadTasks}>Aufgaben laden</button>

{#if error}
    <p>{error}</p>
{:else}
    {#each tasks as task (task.item_id)}
        <p>{task.name}</p>
    {/each}
{/if}