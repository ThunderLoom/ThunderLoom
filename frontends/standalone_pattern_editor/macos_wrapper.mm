#import <Cocoa/Cocoa.h>

void macos_open_file_dialog(
        char * buffer ,
        int bufferSize ,
        const char * aDefaultPathAndFile ,
        const char (*filters)[10],
        const int n_filters) {
    
    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];

    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setLevel:CGShieldingWindowLevel()];
    
    // Set array of file types
    NSMutableArray * fileTypesArray = [NSMutableArray array];
    for (int i = 0;i < n_filters; i++)
    {
        NSString * filt =[NSString stringWithUTF8String:filters[i]];
        [fileTypesArray addObject:filt];
    }

    // Enable options in the dialog.
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowedFileTypes:fileTypesArray];
    [openDlg setAllowsMultipleSelection:FALSE];
    [openDlg setDirectoryURL:[NSURL URLWithString:[NSString stringWithUTF8String:aDefaultPathAndFile ] ] ];

    const char* file;
    if ( [openDlg runModal] == NSModalResponseOK ) {
        NSArray *files = [openDlg URLs];
        // setAllowsMultipleSelection is set to FALSE -> only 1 file
        file = [[[files objectAtIndex:0] path] UTF8String];
        
        //copy file to buffer
        int i = 0;
        for(i = 0; file[i] != '\0'; ++i)
        {
            buffer[i] = file[i];
        }
        buffer[i+1] = '\0';
    }

    [keyWindow makeKeyAndOrderFront:nil];
}

void macos_save_file_dialog(
        char * buffer ,
        int bufferSize ,
        const char * aDefaultPathAndFile,
        const char * ext) {
    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
    
    NSString *suggested_url = [NSString stringWithUTF8String:aDefaultPathAndFile];
    NSString *extention = [NSString stringWithUTF8String:ext];

    NSSavePanel *dialog = [NSSavePanel savePanel];

    printf("ext: %s \t extention: %s \n",ext, [[suggested_url lastPathComponent] UTF8String]);

    [dialog setExtensionHidden:NO];
    [dialog setDirectoryURL:[NSURL URLWithString:[suggested_url stringByDeletingLastPathComponent] ] ];
    [dialog setNameFieldStringValue:[[suggested_url lastPathComponent] stringByAppendingString:extention]];

    const char *utf8Path;
    if ( [dialog runModal] == NSModalResponseOK )
    {
        NSURL *url = [dialog URL];
        utf8Path = [[url path] UTF8String];
        
        //copy file to buffer
        int i = 0;
        for(i = 0; utf8Path[i] != '\0'; ++i)
        {
            buffer[i] = utf8Path[i];
        }
        buffer[i+1] = '\0';
    }
    
    [keyWindow makeKeyAndOrderFront:nil];
}

int macos_alert(const char * details, const char * msg) {
    //Function returns 1 if user presses OK, else 0
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    //[alert addButtonWithTitle:@"Cancel"];
    [alert setMessageText:[NSString stringWithUTF8String:msg]];
    [alert setInformativeText:[NSString stringWithUTF8String:details]];
    [alert setAlertStyle:NSAlertStyleWarning];

    int val = 0;

    if ([alert runModal] == NSAlertFirstButtonReturn) {
        //Ok clicked
        val = 1;
    }
    return val;
}

