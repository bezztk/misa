<script lang="ts">
    import TaskList from "./features/tasks/components/TaskList.svelte";
    import TaskCreate from "./features/tasks/components/TaskCreate.svelte";

    let dialog: HTMLDialogElement;
</script>

<main>
    <h1>Misa</h1>

    <button onclick={() => dialog.showModal()}>Neue Aufgabe</button>

    <dialog bind:this={dialog}>
        <TaskCreate
                onCancel={() => dialog.close()}
                onCreated={() => dialog.close()}
        />
    </dialog>

    <TaskList />
</main>
