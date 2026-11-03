<script lang="ts">
    import { deleteTask, getTasks } from "../../../lib/api/tasks";
    import type { Task } from "../../../lib/models/task";
    import TaskEdit from "./TaskEdit.svelte";

    let tasks: Task[] = $state([]);
    let error = $state("");
    let selectedTask: Task | null = $state(null);
    let dialog: HTMLDialogElement;

    async function loadTasks() {
        try {
            error = "";
            tasks = await getTasks();
        } catch (err) {
            error = String(err);
        }
    }

    function edit(task: Task) {
        selectedTask = task;
        dialog.showModal();
    }
</script>

<button onclick={loadTasks}>Aufgaben laden</button>

{#if error}
    <p>{error}</p>
{:else}
    {#each tasks as task (task.item_id)}
        <p>
            {task.name}
            <button onclick={() => edit(task)}>Bearbeiten</button>
            <button onclick={async () => {
                await deleteTask(task.item_id);
                await loadTasks();
            }}>Löschen</button>
        </p>
    {/each}
{/if}

<dialog bind:this={dialog} onclose={() => selectedTask = null}>
    {#if selectedTask}
        <TaskEdit
                task={selectedTask}
                onCancel={() => dialog.close()}
                onUpdated={async () => {
                    dialog.close();
                    await loadTasks();
                }}
        />
    {/if}
</dialog>
