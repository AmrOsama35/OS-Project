#include <gtk/gtk.h>
#include <stdio.h> // Required for sprintf
#include <string.h> // Required for strcmp
#include <stdlib.h> // Required for malloc and free
#include <glib.h>   // Required for GList and g_list_free
#include <gtk/gtkmenubutton.h> // Required for GtkMenuButton
#include <gtk/gtkpopover.h>    // Required for GtkPopover

// Structure to hold pointers to widgets needed in the button callback
typedef struct {
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *r1_c2_vbox;   // Pointer to the vertical box in R1 C2
    GtkWidget *r1_c4_c2_vbox; // Pointer to the vertical box in R1 C4 C2
    GtkWidget *r1_c4_c3_dropdown_button; // Pointer to the dropdown button in R1 C4 C3 (Renamed)
} ButtonCallbackData;

// typedef struct {
//     GtkWidget *start_button;
//     GtkWidget *stop_button;
//     GtkWidget *reset_button;
// } ControlButtons;

GtkWidget *start_button;
GtkWidget *execution_step_by_step_button;
GtkWidget *execution_auto_button;
GtkWidget *adjust_quantum_button;
GtkWidget *dropdown_button;
GtkWidget *main_vbox;
GtkWidget *window;
GtkWidget *row1;

typedef struct {
    char* process;
    int arrival_time;
} Process;

static int insertionPointer = 0;
// static char *label1 = "Okay";

Process Processes[3];

int shedeluerType = 0;

// Handler for the "destroy" signal of the window
void on_window_destroy(GtkWidget *widget, gpointer data) {
    // Free the callback data structure when the window is destroyed
    // This assumes the callback data is only associated with the window's lifetime
    // and was allocated with malloc.
    if (data != NULL) {
        free(data);
    }
    gtk_main_quit(); // Quit the GTK main loop
}

// void create_start_button() {
//     start_button = gtk_button_new_with_label("Start");
// }

int get_num_of_Processes() {
    int numOfProcesses = 0;
    for (int i = 0; i < insertionPointer; i++)
    {
        if (Processes[i].process != NULL)
        {
            numOfProcesses++;
        }
    }

    return numOfProcesses;
}

void update_sensitivty_of_start_step_auto_button(GtkWidget* button) {
    int numOfProcesses = get_num_of_Processes();
    
    if (numOfProcesses != 0 && shedeluerType != 0)
    {
        gtk_widget_set_sensitive(button, TRUE);
    } else
    {
        gtk_widget_set_sensitive(button, FALSE);
    }    
}

void update_sensitivity_of_adjust_quantum_button() {
 const char* dropdown_button_label = gtk_button_get_label(GTK_BUTTON(dropdown_button));

 if (strcmp(dropdown_button_label, "Round Robin") == 0)
 {
    gtk_widget_set_sensitive(adjust_quantum_button, TRUE);
 } else
 {
     gtk_widget_set_sensitive(adjust_quantum_button, FALSE);
 }   
 
}

// Handler for the buttons inside the scheduler dropdown menu
void on_scheduler_menu_button_clicked(GtkWidget *widget, gpointer data) {
    // data will be the dropdown button (GtkMenuButton)
    //GtkMenuButton *dropdown_button = GTK_MENU_BUTTON(data);
    const char *button_label = gtk_button_get_label(GTK_BUTTON(widget)); // Get label of the clicked button

    if (strcmp(button_label, "FIFO") == 0) {
        shedeluerType = 1;
        gtk_widget_set_sensitive(dropdown_button, FALSE);
    } else if (strcmp(button_label, "Round Robin") == 0) {
        shedeluerType = 2;
    } else {
        shedeluerType = 3;
    }

    update_sensitivty_of_start_step_auto_button(start_button);
    update_sensitivty_of_start_step_auto_button(execution_step_by_step_button);
    update_sensitivty_of_start_step_auto_button(execution_auto_button);

    // Set the label of the dropdown button to the clicked button's label
    gtk_button_set_label(GTK_BUTTON(dropdown_button), button_label);
    update_sensitivity_of_adjust_quantum_button();

    // Get the popover associated with the menu button and hide it
    GtkPopover *popover = gtk_menu_button_get_popover(GTK_MENU_BUTTON(dropdown_button));
    gtk_popover_popdown(popover);

    // // Optional: You could perform an action based on the selected scheduler here
    // printf("Scheduler selected: %s\n", button_label);
}

// Helper function to create a button for the scheduler dropdown menu
GtkWidget* create_scheduler_menu_button(const char* label_text, GtkMenuButton *dropdown_button) {
    GtkWidget *button = gtk_button_new_with_label(label_text);
    // Connect the clicked signal to the handler, passing the dropdown_button pointer
    g_signal_connect(button, "clicked", G_CALLBACK(on_scheduler_menu_button_clicked), dropdown_button);
    // Ensure the button expands horizontally within the menu
    gtk_widget_set_hexpand(button, TRUE);
    return button;
}

// Button click handler for the "Adjust/Save Value" button
void on_adjust_save_button_clicked(GtkWidget *widget, gpointer data) {
    // Cast the generic data pointer back to our specific structure
    ButtonCallbackData *callback_data = (ButtonCallbackData *)data;

    // Get the current label of the button
    const char *current_label = gtk_button_get_label(GTK_BUTTON(callback_data->button));

    gboolean enable_r1_c4_c2_buttons; // Flag to determine if buttons in R1 C4 C2 should be enabled
    gboolean enable_r1_c4_c3_dropdown; // Flag to determine if the R1 C4 C3 dropdown should be enabled
    gboolean enable_r1_c2_buttons;     // Flag to determine if buttons in R1 C2 should be enabled


    if (strcmp(current_label, "Adjust quantum") == 0) {
        // Current state is "Adjust value", transition to "Save value" state
        gtk_widget_set_sensitive(callback_data->entry, TRUE);
        gtk_button_set_label(GTK_BUTTON(callback_data->button), "Save quantum");
        gtk_widget_grab_focus(GTK_WIDGET(callback_data->entry));

        enable_r1_c4_c2_buttons = FALSE;
        enable_r1_c4_c3_dropdown = FALSE; // Disable R1 C4 C3 dropdown
        enable_r1_c2_buttons = FALSE;

    } else { // Assuming the only other state is "Save value"
        // Current state is "Save value", transition back to "Adjust value" state
        gtk_widget_set_sensitive(callback_data->entry, FALSE);
        gtk_button_set_label(GTK_BUTTON(callback_data->button), "Adjust quantum");

        const char* quantum_string = gtk_entry_get_text(GTK_ENTRY(callback_data->entry));
        int quantum = atoi(quantum_string);
        printf("Adjusted Quantum: %d\n", quantum);

        // // Optional: Retrieve the text from the entry field here if needed
        // const char *entered_text = gtk_entry_get_text(GTK_ENTRY(callback_data->entry));
        // printf("Value saved: %s\n", entered_text);

        enable_r1_c4_c2_buttons = TRUE;
        enable_r1_c4_c3_dropdown = TRUE; // Enable R1 C4 C3 dropdown
        enable_r1_c2_buttons = TRUE;
    }

    // Set sensitivity for the R1 C4 C3 dropdown button (New)
    gtk_widget_set_sensitive(callback_data->r1_c4_c3_dropdown_button, enable_r1_c4_c3_dropdown); // Use the new pointer name

    // Set sensitivity for buttons in R1 C2
    GList *children = gtk_container_get_children(GTK_CONTAINER(callback_data->r1_c2_vbox));
    GList *l;
    for (l = children; l != NULL; l = l->next) {
        gtk_widget_set_sensitive(GTK_WIDGET(l->data), enable_r1_c2_buttons);
    }
    g_list_free(children);

    // Set sensitivity for buttons in R1 C4 C2
    children = gtk_container_get_children(GTK_CONTAINER(callback_data->r1_c4_c2_vbox));
    for (l = children; l != NULL; l = l->next) {
        gtk_widget_set_sensitive(GTK_WIDGET(l->data), enable_r1_c4_c2_buttons);
    }
    g_list_free(children);

    // The buttons inside the R1 C4 C3 popover will automatically follow the sensitivity of the GtkMenuButton.
    // No need to iterate through them separately here.
}


// Function to create a basic placeholder block (e.g., a Frame with a Label)
// This function is used by the column/row functions to create their content.
// We keep this function as it's still used for other blocks.
GtkWidget* create_block(GtkWidget* label) {
    GtkWidget *frame = gtk_frame_new(NULL); // Create a frame with no title
    // Use an inner shadow for the innermost blocks
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    //GtkWidget *label = gtk_label_new(label_text); // Create a label with the given text
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5); // Add padding inside the frame

    // Add the label to the frame
    gtk_container_add(GTK_CONTAINER(frame), label);

    // Set expand and fill properties so the block tries to fill its allocated space
    // These can be overridden by the packing options in parent containers.
    gtk_widget_set_vexpand(frame, TRUE); // Allow vertical expansion by default
    gtk_widget_set_hexpand(frame, TRUE); // Allow horizontal expansion by default

    gtk_widget_set_halign(frame, GTK_ALIGN_FILL);
    gtk_widget_set_valign(frame, GTK_ALIGN_FILL);

    return frame;
}

// --- Modal Dialog Functions ---

// Creates and runs the "Add Process" modal dialog
void create_add_process_modal(GtkWindow *parent_window) {
    // Create a new modal dialog
    // Arguments: title, parent window, flags (modal, destroy-with-parent), button text, response ID, ... NULL
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Process",
                                                    parent_window,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Load Process",
                                                    GTK_RESPONSE_ACCEPT, // Use ACCEPT for the Load button
                                                    NULL); // Sentinel NULL

    // Set a default size for the dialog
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); // Make dialog not resizable

    // Get the content area of the dialog (where we can pack our widgets)
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Create a vertical box to arrange the label, entry, and button inside the dialog
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // 10px spacing

    // Create the label for the input field
    GtkWidget *label_file = gtk_label_new("Enter File Path below:");

    // Create the input field (GtkEntry)
    GtkWidget *entry_file = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_file), "e.g., /path/to/process_file.txt");

    GtkWidget *label_arrival = gtk_label_new("Enter Arrival Time below:");

    GtkWidget *entry_arrival = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_arrival), "e.g., 1");

    // Pack the label and entry into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), label_file, FALSE, FALSE, 0); // Label doesn't expand
    gtk_box_pack_start(GTK_BOX(vbox), entry_file, TRUE, TRUE, 0);   // Entry expands and fills
    gtk_box_pack_start(GTK_BOX(vbox), label_arrival, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_arrival, TRUE, TRUE, 0);

    // Add some padding around the content inside the dialog
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);


    // Add the vertical box containing the label and entry to the dialog's content area
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Show all widgets in the dialog before running it
    gtk_widget_show_all(dialog);

    // Run the dialog. This function blocks until the dialog is closed.
    // The return value is the response ID of the button that was clicked.
    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    // Handle the response (e.g., if "Load Process" was clicked)
    if (response == GTK_RESPONSE_ACCEPT) {
        // The "Load Process" button was clicked
        // You could retrieve the text from the entry here if needed
        const char *file_path = gtk_entry_get_text(GTK_ENTRY(entry_file));
        printf("File path entered: %s\n", file_path);

        const char *arrival_time_string = gtk_entry_get_text(GTK_ENTRY(entry_arrival));
        const int arrival_time = atoi(arrival_time_string);
        printf("Arrival Time entered: %d\n", arrival_time);
        // In a real application, you would process the file path here

        Process process;
        process.process = malloc(128);
        strcpy(process.process, file_path);
        process.arrival_time = arrival_time;

        Processes[insertionPointer] = process;
        insertionPointer++;

        update_sensitivty_of_start_step_auto_button(start_button);
        update_sensitivty_of_start_step_auto_button(execution_step_by_step_button);
        update_sensitivty_of_start_step_auto_button(execution_auto_button);
    }

    // Destroy the dialog widget when gtk_dialog_run returns
    gtk_widget_destroy(dialog);
}

void on_process_individual_config_button_clicked(GtkWidget* button, GtkWidget* entry) {
    const char *arrival_time_string = gtk_entry_get_text(GTK_ENTRY(entry));
    const int arrival_time = atoi(arrival_time_string);
    printf("Arrival Time entered: %d\n", arrival_time);
}

// Creates and runs the "Process Configuration" modal dialog
void create_process_config_modal(GtkWindow *parent_window) {
     // Create a new modal dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Process Configuration",
                                                    parent_window,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    NULL, // No standard buttons initially
                                                    NULL); // Sentinel NULL

    // Set a default size for the dialog
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 200); // Adjust size as needed
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); // Make dialog not resizable


    // Get the content area of the dialog
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Create a horizontal box to hold the three columns
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); // 10px spacing between columns
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE); // Make columns divide space equally

    // Add some padding around the content inside the dialog
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

    // --- Create and pack the content for each of the three columns ---
    for (int i = 1; i <= 3; ++i) {
        // Create a vertical box for each column
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 5px spacing between items in column
        gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Items in column don't need equal height

        // Create the "Process:" label
        GtkWidget *label = gtk_label_new("Process:");

        // Create the input field (GtkEntry)
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Adjust the arrival time");

        // Create the "Adjust Arrival Time" button
        GtkWidget *button = gtk_button_new_with_label("Adjust Arrival Time");

        g_signal_connect(button, "clicked", G_CALLBACK(on_process_individual_config_button_clicked), entry);

        // Pack the widgets into the column's vertical box
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0); // Label doesn't expand
        gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);   // Entry doesn't expand vertically much by default
        gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0); // Button doesn't expand vertically

        // Pack the column's vertical box into the main horizontal box
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0); // Column expands and fills horizontally
    }


    // Add the main horizontal box to the dialog's content area
    gtk_container_add(GTK_CONTAINER(content_area), hbox);

    // Show all widgets in the dialog before running it
    gtk_widget_show_all(dialog);

    // Run the dialog. This function blocks until the dialog is closed.
    // GTK_RESPONSE_NONE is returned if the dialog is destroyed without clicking a button
    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    // Handle the response if needed (e.g., if you added other buttons later)
    // if (response == SOME_RESPONSE_ID) { ... }

    // Destroy the dialog widget when gtk_dialog_run returns
    gtk_widget_destroy(dialog);
}


// --- Functions for creating content within columns in Row 1 ---

// Row 1, Column 1: Remains a single block
GtkWidget* create_column1_row1_content(GtkWidget *label1) { 
    return create_block(label1); 
}

// Row 1, Column 2: Split into 2 rows, now using specific Buttons that expand vertically
GtkWidget* create_column2_row1_content() {
    // Create a vertical box to hold the 2 buttons, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    // Set homogeneous to TRUE so buttons divide vertical space equally
    gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE); // Changed to TRUE for equal vertical division

    // Create the two buttons
    execution_step_by_step_button = gtk_button_new_with_label("Execution Step-by-Step");
    execution_auto_button = gtk_button_new_with_label("Execution Auto");

    update_sensitivty_of_start_step_auto_button(execution_step_by_step_button);
    update_sensitivty_of_start_step_auto_button(execution_auto_button);

    // Pack the buttons into the vertical box
    // Buttons should expand vertically (TRUE) and fill horizontally (TRUE)
    gtk_box_pack_start(GTK_BOX(vbox), execution_step_by_step_button, TRUE, TRUE, 0); // Changed expand to TRUE
    gtk_box_pack_start(GTK_BOX(vbox), execution_auto_button, TRUE, TRUE, 0); // Changed expand to TRUE

    // Set expand and fill properties for the vbox itself within its parent
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);

    return vbox; // Return the vertical box containing the two buttons
}

// Handler for the "Add Process" button click
void on_add_process_button_clicked(GtkWidget *widget, gpointer data) {
    // data will be the main window widget
    GtkWindow *main_window = GTK_WINDOW(data);
    // Call the function to create and run the modal dialog
    create_add_process_modal(main_window);
}

// Handler for the "Process Configuration" button click
void on_process_config_button_clicked(GtkWidget *widget, gpointer data) {
    // data will be the main window widget
    GtkWindow *main_window = GTK_WINDOW(data);
    // Call the function to create and run the process configuration modal dialog
    create_process_config_modal(main_window);
}


// Row 1, Column 3: Split into 2 rows, now using specific Buttons that expand vertically
// The top button ("Add Process") will trigger the modal
GtkWidget* create_column3_row1_content(GtkWindow *main_window) { // Accept main_window pointer
    // Create a vertical box to hold the 2 buttons, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    // Set homogeneous to TRUE so buttons divide vertical space equally
    gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE); // Changed to TRUE for equal vertical division

    // Create the two buttons
    GtkWidget *button_add = gtk_button_new_with_label("Add Process");
    GtkWidget *button_config = gtk_button_new_with_label("Process Configuration");

    // Connect the "Add Process" button to the modal handler, passing the main window
    g_signal_connect(button_add, "clicked", G_CALLBACK(on_add_process_button_clicked), main_window);
    // Connect the "Process Configuration" button to its modal handler, passing the main window
    g_signal_connect(button_config, "clicked", G_CALLBACK(on_process_config_button_clicked), main_window);


    // Pack the buttons into the vertical box
    // Buttons should expand vertically (TRUE) and fill horizontally (TRUE)
    gtk_box_pack_start(GTK_BOX(vbox), button_add, TRUE, TRUE, 0); // Changed expand to TRUE
    gtk_box_pack_start(GTK_BOX(vbox), button_config, TRUE, TRUE, 0); // Changed expand to TRUE

    // Set expand and fill properties for the vbox itself within its parent
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);

    return vbox; // Return the vertical box containing the two buttons
}

// --- Functions for creating content within sub-columns of Row 1, Column 4 ---

void on_stop_clicked(GtkButton *button, gpointer data) {
    GtkLabel *label_to_update = GTK_LABEL(data);
    gtk_label_set_text(label_to_update, "stopped");
}

// Row 1, Column 4, Column 1: Contains an input field and a button
// Now accepts pointers to the other two vboxes in R1 C4 and R1 C2
GtkWidget* create_column1_row1_col4_content(GtkWidget *r1_c2_vbox, GtkWidget *r1_c4_c2_vbox, GtkWidget *r1_c4_c3_dropdown_button) { // Added r1_c2_vbox
    // Create a vertical box to hold the entry and button
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);

    // Create the input field (GtkEntry)
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter quantum here...");
    // Initially disable the input field
    gtk_widget_set_sensitive(entry, FALSE);

    // Create the button
    adjust_quantum_button = gtk_button_new_with_label("Adjust quantum");
    update_sensitivity_of_adjust_quantum_button();

    // Create a structure to pass all necessary widgets to the callback
    // Note: In a real application, you might need to free this struct
    // when the widgets are destroyed if they are dynamically created/destroyed.
    ButtonCallbackData *callback_data = malloc(sizeof(ButtonCallbackData));
    callback_data->entry = entry;
    callback_data->button = adjust_quantum_button;
    callback_data->r1_c2_vbox = r1_c2_vbox;       // Store pointer to R1 C2 vbox (New)
    callback_data->r1_c4_c2_vbox = r1_c4_c2_vbox; // Store pointer to C2 vbox
    callback_data->r1_c4_c3_dropdown_button = r1_c4_c3_dropdown_button; // Store pointer to C3 dropdown button


    // Connect the button's "clicked" signal to the handler, passing the struct
    g_signal_connect(adjust_quantum_button, "clicked", G_CALLBACK(on_adjust_save_button_clicked), callback_data);

    // Pack the entry and button into the vertical box
    // Entry should expand and fill
    gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
    // Button should not necessarily expand vertically, but fill horizontally
    gtk_box_pack_start(GTK_BOX(vbox), adjust_quantum_button, FALSE, TRUE, 0);

    // Set expand and fill properties for the vbox itself within its parent
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);


    return vbox; // Return the vertical box containing the entry and button
}



// Row 1, Column 4, Column 2: Split into 3 rows, using Buttons directly
GtkWidget* create_column2_row1_col4_content(GtkWidget *label1) {
    // Create a vertical box to hold the 3 buttons (representing rows), with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow buttons to have different sizes if needed

    // Pack the three buttons directly into the vertical box (no extra frame for padding)
    start_button = gtk_button_new_with_label("Start");
    GtkWidget *stop_button = gtk_button_new_with_label("Stop");
    update_sensitivty_of_start_step_auto_button(start_button);
    gtk_box_pack_start(GTK_BOX(vbox), start_button, TRUE, TRUE, 0);
    g_signal_connect(stop_button, "clicked", G_CALLBACK(on_stop_clicked), label1);
    gtk_box_pack_start(GTK_BOX(vbox), stop_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_button_new_with_label("Reset"), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the three buttons
}

// Row 1, Column 4, Column 3: Now a dropdown button with scheduler options
void create_column3_row1_col4_content() {
    // Create the dropdown button
    dropdown_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(dropdown_button), "Choose Scheduler"); // Initial label

    // Create the popover that will contain the menu items
    GtkWidget *popover = gtk_popover_new(dropdown_button); // Popover is attached to the dropdown button

    // Create a vertical box to hold the buttons inside the popover
    GtkWidget *vbox_menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 5px spacing

    // Create the three buttons for the scheduler options and pack them into the vbox
    // Pass the dropdown_button pointer to the helper function so the callback can access it
    gtk_box_pack_start(GTK_BOX(vbox_menu), create_scheduler_menu_button("FIFO", GTK_MENU_BUTTON(dropdown_button)), FALSE, FALSE, 0); // Corrected label
    gtk_box_pack_start(GTK_BOX(vbox_menu), create_scheduler_menu_button("Round Robin", GTK_MENU_BUTTON(dropdown_button)), FALSE, FALSE, 0); // Corrected label
    gtk_box_pack_start(GTK_BOX(vbox_menu), create_scheduler_menu_button("MLFQ", GTK_MENU_BUTTON(dropdown_button)), FALSE, FALSE, 0); // Corrected label

    // *** FIX: Show the widgets inside the popover's content area ***
    gtk_widget_show_all(vbox_menu);

    // Set the vbox as the child of the popover
    gtk_container_add(GTK_CONTAINER(popover), vbox_menu);

    // Set the popover for the menu button
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(dropdown_button), popover);

    // Set expand and fill properties for the dropdown button itself within its parent
    gtk_widget_set_vexpand(dropdown_button, TRUE); // Allow vertical expansion
    gtk_widget_set_hexpand(dropdown_button, TRUE); // Allow horizontal expansion

    // return dropdown_button; // Return the dropdown button
}


// Row 1, Column 4: Split into 3 columns (using the sub-column content functions above)
// Now accepts the R1 C2 vbox pointer
GtkWidget* create_column4_row1_content(GtkWidget *r1_c2_vbox, GtkWidget *label1) {
    // Create a horizontal box to hold the 3 sub-columns, with 5px spacing
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE); // Allow columns to have different sizes

    // Create the vboxes for the button columns first
    GtkWidget *r1_c4_c2_vbox = create_column2_row1_col4_content(label1);
    // Create the dropdown button for R1 C4 C3 (New)
    // GtkWidget *r1_c4_c3_dropdown_button = create_column3_row1_col4_content(); // Call the updated function
    create_column3_row1_col4_content();
    GtkWidget *r1_c4_c3_dropdown_button = dropdown_button; // Call the updated function

    // Create the content for the first column, passing the other vboxes and the dropdown button
    GtkWidget *r1_c4_c1_content = create_column1_row1_col4_content(r1_c2_vbox, r1_c4_c2_vbox, r1_c4_c3_dropdown_button); // Pass dropdown button

    // Pack the three sub-column blocks/containers into the horizontal box
    gtk_box_pack_start(GTK_BOX(hbox), r1_c4_c1_content, TRUE, TRUE, 0); // Vertical box with Entry and Button
    gtk_box_pack_start(GTK_BOX(hbox), r1_c4_c2_vbox, TRUE, TRUE, 0); // Vertical box with 3 buttons
    gtk_box_pack_start(GTK_BOX(hbox), r1_c4_c3_dropdown_button, TRUE, TRUE, 0); // Pack the dropdown button (New)

    return hbox; // Return the horizontal box containing the three sub-columns
}


// Function to create Row 1: contains 4 columns with specified subdivisions, each wrapped in a border frame
// It takes the parent vertical box (main_vbox) and main_window as arguments
// void create_row1(GtkWidget *parent_vbox, GtkWindow *main_window) {
//     // Create a horizontal box to hold the 4 main columns of Row 1, with 5px spacing between column frames
//     GtkWidget *row1_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
//     gtk_widget_set_size_request(row1_hbox, -1, 10);
//     // Set homogeneous to FALSE so columns can potentially have different sizes
//     gtk_box_set_homogeneous(GTK_BOX(row1_hbox), FALSE);

//     // --- Create and pack the content for each of the 4 main columns, wrapped in a Frame for a border ---

//     // Column 1
//     GtkWidget *col1_frame = gtk_frame_new(NULL);
//     // Use an etched out shadow for the main column borders
//     gtk_frame_set_shadow_type(GTK_FRAME(col1_frame), GTK_SHADOW_ETCHED_OUT);
//     gtk_container_set_border_width(GTK_CONTAINER(col1_frame), 5); // Padding inside the column frame
//     gtk_container_add(GTK_CONTAINER(col1_frame), create_column1_row1_content()); // Add column content to the frame
//     gtk_box_pack_start(GTK_BOX(row1_hbox), col1_frame, TRUE, TRUE, 0); // Pack the frame into the row HBox

//     // Column 2 (Need to get the vbox here to pass to R1 C4 C1)
//     GtkWidget *r1_c2_vbox = create_column2_row1_content(); // Create the vbox for R1 C2
//     GtkWidget *col2_frame = gtk_frame_new(NULL);
//     gtk_frame_set_shadow_type(GTK_FRAME(col2_frame), GTK_SHADOW_ETCHED_OUT);
//     gtk_container_set_border_width(GTK_CONTAINER(col2_frame), 5);
//     gtk_container_add(GTK_CONTAINER(col2_frame), r1_c2_vbox); // Add the vbox to the frame
//     gtk_box_pack_start(GTK_BOX(row1_hbox), col2_frame, TRUE, TRUE, 0);

//     // Column 3
//     GtkWidget *col3_frame = gtk_frame_new(NULL);
//     gtk_frame_set_shadow_type(GTK_FRAME(col3_frame), GTK_SHADOW_ETCHED_OUT);
//     gtk_container_set_border_width(GTK_CONTAINER(col3_frame), 5);
//     gtk_container_add(GTK_CONTAINER(col3_frame), create_column3_row1_content(main_window)); // Pass main_window to create_column3_row1_content
//     gtk_box_pack_start(GTK_BOX(row1_hbox), col3_frame, TRUE, TRUE, 0);

//     // Column 4 (Pass the R1 C2 vbox to create_column4_row1_content)
//     GtkWidget *col4_frame = gtk_frame_new(NULL);
//     gtk_frame_set_shadow_type(GTK_FRAME(col4_frame), GTK_SHADOW_ETCHED_OUT);
//     gtk_container_set_border_width(GTK_CONTAINER(col4_frame), 5);
//     gtk_container_add(GTK_CONTAINER(col4_frame), create_column4_row1_content(r1_c2_vbox)); // Passed r1_c2_vbox
//     gtk_box_pack_start(GTK_BOX(row1_hbox), col4_frame, TRUE, TRUE, 0);

//     // Pack the Row 1 horizontal box into the parent vertical box (main_vbox).
//     gtk_box_pack_start(GTK_BOX(parent_vbox), row1_hbox, TRUE, TRUE, 5); // 5px padding around the row
// }

// Function to create Row 1: contains 4 columns with specified subdivisions, each wrapped in a border frame
// It takes the parent vertical box (main_vbox) and main_window as arguments
void create_row1(GtkWidget *parent_vbox, GtkWindow *main_window) {
    // Create a horizontal box to hold the 4 main columns of Row 1, with 5px spacing between column frames
    // GtkWidget *row1_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    // gtk_widget_set_size_request(row1_hbox, -1, 10);
    // Set homogeneous to FALSE so columns can potentially have different sizes
    // gtk_box_set_homogeneous(GTK_BOX(row1_hbox), FALSE);

    // --- Create and pack the content for each of the 4 main columns, wrapped in a Frame for a border ---


    GtkWidget *label1 = gtk_label_new("okay");
    row1 = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(row1), 10);    // 10px spacing between rows
    gtk_grid_set_column_spacing(GTK_GRID(row1), 10);
    // Column 1
    GtkWidget *col1_frame = gtk_frame_new(NULL);
    //Use an etched out shadow for the main column borders
    gtk_frame_set_shadow_type(GTK_FRAME(col1_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col1_frame), 5); // Padding inside the column frame
    gtk_container_add(GTK_CONTAINER(col1_frame), create_column1_row1_content(label1)); // Add column content to the frame
    gtk_grid_attach(GTK_GRID(row1), col1_frame, 0, 0, 1, 1);
    // gtk_box_pack_start(GTK_BOX(row1_hbox), col1_frame, TRUE, TRUE, 0); // Pack the frame into the row HBox

    // Column 2 (Need to get the vbox here to pass to R1 C4 C1)
    GtkWidget *r1_c2_vbox = create_column2_row1_content(); // Create the vbox for R1 C2
    GtkWidget *col2_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col2_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col2_frame), 5);
    gtk_container_add(GTK_CONTAINER(col2_frame), r1_c2_vbox); // Add the vbox to the frame
    gtk_grid_attach(GTK_GRID(row1), col2_frame, 1, 0, 1, 1);
    //gtk_box_pack_start(GTK_BOX(row1_hbox), col2_frame, TRUE, TRUE, 0);

    // Column 3
    GtkWidget *col3_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col3_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col3_frame), 5);
    gtk_container_add(GTK_CONTAINER(col3_frame), create_column3_row1_content(main_window)); // Pass main_window to create_column3_row1_content
    gtk_grid_attach(GTK_GRID(row1), col3_frame, 2, 0, 1, 1);
    //gtk_box_pack_start(GTK_BOX(row1_hbox), col3_frame, TRUE, TRUE, 0);

    // Column 4 (Pass the R1 C2 vbox to create_column4_row1_content)
    GtkWidget *col4_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col4_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col4_frame), 5);
    gtk_container_add(GTK_CONTAINER(col4_frame), create_column4_row1_content(r1_c2_vbox, label1)); // Passed r1_c2_vbox
    gtk_grid_attach(GTK_GRID(row1), col4_frame, 3, 0, 1, 1);
    //gtk_box_pack_start(GTK_BOX(row1_hbox), col4_frame, TRUE, TRUE, 0);

    // Pack the Row 1 horizontal box into the parent vertical box (main_vbox).
    gtk_box_pack_start(GTK_BOX(parent_vbox), row1, TRUE, TRUE, 5); // 5px padding around the row
}


// --- Functions for creating content within columns in Row 2 ---

// Row 2, Column 1: Split into 3 rows
GtkWidget* create_column1_row2_content() {
    // Create a vertical box to hold the 3 rows, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    // Pack the three blocks (representing rows) into the vertical box
    GtkWidget *label1 = gtk_label_new("R2 C1 r1");
    GtkWidget *label2 = gtk_label_new("R2 C1 r2");
    GtkWidget *label3 = gtk_label_new("R2 C1 r3");
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label1), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label2), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label3), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the three rows
}

// Row 2, Column 2: Split into 3 rows
GtkWidget* create_column2_row2_content() {
    // Create a vertical box to hold the 3 rows, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    GtkWidget *label1 = gtk_label_new("R2 C2 r1");
    GtkWidget *label2 = gtk_label_new("R2 C2 r2");
    GtkWidget *label3 = gtk_label_new("R2 C2 r3");
    // Pack the three blocks (representing rows) into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label1), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label2), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label3), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the three rows
}

// Row 2, Column 3: Split into 4 rows
GtkWidget* create_column3_row2_content() {
    // Create a vertical box to hold the 4 rows, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    GtkWidget *label1 = gtk_label_new("R2 C3 r1");
    GtkWidget *label2 = gtk_label_new("R2 C3 r2");
    GtkWidget *label3 = gtk_label_new("R2 C3 r3");
    GtkWidget *label4 = gtk_label_new("R2 C3 r4");
    // Pack the four blocks (representing rows) into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label1), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label2), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label3), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block(label4), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the four rows
}

// Row 2, Column 4: Split into 60 rows
GtkWidget* create_column4_row2_content() {
    // Create a vertical box to hold the 60 rows, with 2px spacing (less spacing for many rows)
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    // Pack the sixty blocks (representing rows) into the vertical box
    for (int i = 1; i <= 60; ++i) {
        char label_text[20]; // Buffer for label text
        sprintf(label_text, "R2 C4 R%d", i); // Format the label text
        GtkWidget *label = gtk_label_new(label_text);
        // For many rows, pack with expand=FALSE, fill=FALSE so they don't force excessive vertical size
        // This column might require a GtkScrolledWindow if the content exceeds the window height.
        gtk_box_pack_start(GTK_BOX(vbox), create_block(label), FALSE, FALSE, 0);
    }

    return vbox; // Return the vertical box containing the sixty rows
}

// Function to create Row 2: contains 4 columns with specified subdivisions, each wrapped in a border frame
// It takes the parent vertical box (main_vbox) as an argument
void create_row2(GtkWidget *parent_vbox) {
    // Create a horizontal box to hold the 4 main columns of Row 2, with 5px spacing between column frames
    GtkWidget *row2_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    // Set homogeneous to FALSE so columns can potentially have different sizes
    gtk_box_set_homogeneous(GTK_BOX(row2_hbox), FALSE);

    // --- Create and pack the content for each of the 4 main columns, wrapped in a Frame for a border ---

    // Column 1
    GtkWidget *col1_frame = gtk_frame_new(NULL);
    // Use an etched out shadow for the main column borders
    gtk_frame_set_shadow_type(GTK_FRAME(col1_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col1_frame), 5); // Padding inside the column frame
    gtk_container_add(GTK_CONTAINER(col1_frame), create_column1_row2_content()); // Add column content to the frame
    gtk_box_pack_start(GTK_BOX(row2_hbox), col1_frame, TRUE, TRUE, 0); // Pack the frame into the row HBox

    // Column 2
    GtkWidget *col2_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col2_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col2_frame), 5);
    gtk_container_add(GTK_CONTAINER(col2_frame), create_column2_row2_content());
    gtk_box_pack_start(GTK_BOX(row2_hbox), col2_frame, TRUE, TRUE, 0);

    // Column 3
    GtkWidget *col3_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col3_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col3_frame), 5);
    gtk_container_add(GTK_CONTAINER(col3_frame), create_column3_row2_content());
    gtk_box_pack_start(GTK_BOX(row2_hbox), col3_frame, TRUE, TRUE, 0);

    // Column 4
    GtkWidget *col4_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col4_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col4_frame), 5);
    gtk_container_add(GTK_CONTAINER(col4_frame), create_column4_row2_content());
     // For the column with many rows, allow its frame to expand horizontally,
     // but its vertical size will be primarily determined by its packed children (which are FALSE, FALSE).
    gtk_box_pack_start(GTK_BOX(row2_hbox), col4_frame, TRUE, TRUE, 0);

    // Pack the Row 2 horizontal box into the parent vertical box (main_vbox).
    gtk_box_pack_start(GTK_BOX(parent_vbox), row2_hbox, TRUE, TRUE, 5); // 5px padding around the row
}

// Function to create Row 3: contains a single block
// It takes the parent vertical box (main_vbox) as an argument
// This function remains the same as before
void create_row3(GtkWidget *parent_vbox) {
    // Row 3 is a single block that spans the width
    GtkWidget *label = gtk_label_new("Row 3");
    GtkWidget *row3_block = create_block(label);

    // Pack the Row 3 block into the parent vertical box (main_vbox).
    // TRUE, TRUE means this row will expand and fill vertically and horizontally.
    gtk_box_pack_start(GTK_BOX(parent_vbox), row3_block, TRUE, TRUE, 5); // 5px padding around the row
}

// Function to initialize and set up the main GTK application window and its contents
// This function creates the main window, the scrolled window, the primary layout container (main_vbox),
// and then calls the functions to create and pack the rows.
GtkWidget* create_main_window() {
    // Create a new top-level window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Complex Layout with Scheduler Dropdown"); // Updated window title
    // No border width directly on the window's content area when using a scrolled window here.
    // The scrolled window will contain the main_vbox which has its own padding/spacing.
    // gtk_container_set_border_width(GTK_CONTAINER(window), 10); // Removed or set to 0

    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600); // Set a default window size

    // Connect the "destroy" signal to our handler to quit the app when the window is closed
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create a Scrolled Window to make the content scrollable
    // The NULL, NULL arguments mean use default scroll adjustments
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    // Set the scrollbar policy:
    // GTK_POLICY_NEVER: Never show horizontal scrollbar
    // GTK_POLICY_AUTOMATIC: Show vertical scrollbar only when needed (content exceeds height)
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    // Add the scrolled window to the main window
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);


    // Create a main vertical box to hold the three rows.
    // 5px spacing between the rows.
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    //gtk_box_set_homogeneous(GTK_BOX(main_vbox), FALSE);
    // Add the main vertical box to the Scrolled Window
    // The scrolled window can only contain one child.
    gtk_container_add(GTK_CONTAINER(scrolled_window), main_vbox);


    // int numOfProcesses = get_num_of_Processes();
    // Create the rows and add them to the main VBox
    // We pass main_vbox as the parent container for the rows.
    // Pass the main window pointer to create_row1 so it can be used for the modal
    create_row1(main_vbox, GTK_WINDOW(window));
    // if (numOfProcesses == 0)
    // {
    //     gtk_box_pack_start(GTK_BOX(main_vbox), create_block("Start By Adding a process"), TRUE, TRUE, 5);
    // } else {   
        create_row2(main_vbox);
        create_row3(main_vbox);
    //}

    const char* dropdown_button_label = gtk_button_get_label(GTK_BUTTON(dropdown_button));
    printf("Dropdown label: %s\n", dropdown_button_label);
    // Return the created window widget
    return window;
}

int main(int argc, char *argv[]) {
    GtkWidget *window; // Declare the main window widget pointer

    // Initialize GTK. This must be called before using any other GTK functions.
    // It processes GTK-specific command-line arguments and initializes the toolkit.
    gtk_init(&argc, &argv);

    // Create and set up the main application window and its contents by calling our setup function.
    window = create_main_window();

    // Show all widgets contained within the window (the window itself, the scrolled window,
    // the vbox, hboxes, frames, labels).
    gtk_widget_show_all(window);

    // Start the GTK main loop. This function enters the main processing loop and
    // waits for events (like user input, window events, timer events).
    // The application will stay running here until gtk_main_quit() is called.
    gtk_main();

    // The program exits when gtk_main() returns.
    return 0;
}
