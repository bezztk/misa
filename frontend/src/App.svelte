<script lang="ts">
    import TaskList from "./features/tasks/components/TaskList.svelte";
    import TaskCreate from "./features/tasks/components/TaskCreate.svelte";
    import DevelopmentView from "./features/development/components/DevelopmentView.svelte";

    let dialog = $state<HTMLDialogElement>();
    let taskList = $state<TaskList>();
    let view = $state<"tasks" | "development">("tasks");
</script>

<main>
    <h1>Misa</h1>

    <nav>
        <button onclick={() => view = "tasks"}>Aufgaben</button>
        <button onclick={() => view = "development"}>Entwicklung</button>
    </nav>

    {#if view === "tasks"}
        <button onclick={() => dialog?.showModal()}>Neue Aufgabe</button>

        <dialog bind:this={dialog}>
            <TaskCreate
                    onCancel={() => dialog?.close()}
                    onCreated={() => {
                        dialog?.close();
                        void taskList?.loadTasks();
                    }}
            />
        </dialog>

        <TaskList bind:this={taskList} />
    {:else}
        <DevelopmentView />
    {/if}
</main>

<style>
    main {
        max-width: 50rem;
        margin: 0 auto;
        padding: 1rem;
    }

    nav {
        display: flex;
        gap: 0.5rem;
        margin-bottom: 1rem;
    }
</style>
