<script lang="ts">
    import {onMount} from "svelte";
    import {deleteTask, getTaskMetadata, getTasks} from "../../../lib/api/tasks";
    import type { Task } from "../../../lib/models/task";
    import type {TaskMetadata} from "../../../lib/models/taskMetadata";
    import TaskEdit from "./TaskEdit.svelte";

    let tasks: Task[] = $state([]);
    let metadata: TaskMetadata | null = $state(null);
    let error = $state("");
    let selectedTask: Task | null = $state(null);
    let dialog: HTMLDialogElement;

    export async function loadTasks() {
        try {
            error = "";
            tasks = await getTasks();
        } catch (err) {
            error = String(err);
        }
    }

    onMount(async () => {
        metadata = await getTaskMetadata();
        await loadTasks();
    });

    function edit(task: Task) {
        selectedTask = task;
        dialog.showModal();
    }
</script>

<section>
    <h2>Aufgaben</h2>

    {#if error}
        <p>{error}</p>
    {:else}
        {#each tasks as task (task.item_id)}
            <article>
                <h3>{task.name}</h3>

                {#if task.description}
                    <p>{task.description}</p>
                {/if}

                {#if task.state_id !== null}
                    <p>Status: {metadata?.states.find(state => state.id === task.state_id)?.name}</p>
                {/if}

                {#if task.group_id !== null}
                    <p>Gruppe: {metadata?.groups.find(group => group.id === task.group_id)?.name}</p>
                {/if}

                <button onclick={() => edit(task)}>Bearbeiten</button>
                <button onclick={async () => {
                    await deleteTask(task.item_id);
                    await loadTasks();
                }}>Löschen</button>
            </article>
        {/each}
    {/if}
</section>

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

<style>
    section {
        margin-top: 2rem;
    }

    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    article h3,
    article p {
        margin: 0 0 0.5rem;
    }

    article button {
        margin-right: 0.5rem;
    }
</style>
