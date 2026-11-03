<script lang="ts">
    import { getTasks, type Task } from "./lib/api/tasks";

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

<main>
    <h1>Misa</h1>

    <button onclick={loadTasks}>Aufgaben laden</button>

    {#if error}
        <p>{error}</p>
    {:else}
        {#each tasks as task (task.id)}
            <p>{task.title}</p>
        {/each}
    {/if}
</main>