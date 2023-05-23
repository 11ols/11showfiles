/**
	
*/

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object
#include "common/commonsyms.c"

#ifdef WIN_VERSION

#include <Shlobj.h>

#else
#import <AppKit/AppKit.h>
#endif


////////////////////////// object struct
typedef struct _showfile
{
	t_object	ob;
    
} t_showfile;

///////////////////////// function prototypes
void *showfile_new(t_symbol *s, long argc, t_atom *argv);
void showfile_free(t_showfile *x);
void showfile_assist(t_showfile *x, void *b, long m, long a, char *s);

void showfile_show(t_showfile *x, t_symbol *s, long ac, t_atom *av);
void showfile_do_show(t_showfile *x, t_symbol* s, long ac, t_atom* av);


//////////////////////// global class pointer variable
void *showfile_class;


void ext_main(void *r)
{
	common_symbols_init();
	t_class *c;

	c = class_new("11showfiles", (method)showfile_new, (method)showfile_free, (long)sizeof(t_showfile),
				  0L /* leave NULL!! */, A_GIMME, 0);

	class_addmethod(c, (method)showfile_show,		"show",	A_GIMME, 0);
	class_addmethod(c, (method)showfile_assist,			"assist",		A_CANT, 0);
	
	class_register(CLASS_BOX, c);
	showfile_class = c;
    
    object_post(NULL, "11showfiles 2023/03/26 11OLSEN.DE");
}


#ifndef WIN_VERSION

void showfile_show(t_showfile* x, t_symbol* s, long ac, t_atom* av)
{
	defer_low(x, (method)showfile_do_show, s, ac, av);
}
void showfile_do_show(t_showfile *x, t_symbol* s, long argc, t_atom* argv)
{
    int i;
    
    if (!argc) return;
    
    for (i = 0; i < argc; i++)
    {
        if (atom_gettype(argv + i) != A_SYM)
        {
            object_error((t_object *)x, "wrong input, please use a list of file path symbols");
            return;
        }
    }
    
    NSMutableArray *paths = [[NSMutableArray alloc] initWithCapacity:argc];
    
    for (i = 0; i < argc; i++)
    {
        char thePath[MAX_PATH_CHARS];
        t_symbol* absPath;
        
        
        if (path_absolutepath(&absPath, atom_getsym(argv + i), NULL, 0))
        {
            object_error((t_object *)x, "The file “%s” does not exist. Ignoring.\n", atom_getsym(argv + i)->s_name);
            continue;
        }
        
        path_nameconform(absPath->s_name, thePath, PATH_STYLE_NATIVE, PATH_TYPE_BOOT);
        
        NSString* pathStr = [NSString stringWithUTF8String:thePath];
        
        NSURL *fileURL = [NSURL fileURLWithPath:pathStr];

        [paths addObject:fileURL];
    }
    
    [[NSWorkspace sharedWorkspace]  activateFileViewerSelectingURLs:paths];
    
    [paths release];
}

#else


void showfile_show(t_showfile* x, t_symbol* s, long ac, t_atom* av)
{
    defer_low(x, (method)showfile_do_show, s, ac, av);
}
void showfile_do_show(t_showfile *x, t_symbol* s, long argc, t_atom* argv)
{
    int i;

    if (!argc) return;

    for (i = 0; i < argc; i++)
    {
        if (atom_gettype(argv + i) != A_SYM)
        {
            object_error((t_object*)x, "wrong input, please use a list of file path symbols");
            return;
        }
    }

    if (argc == 1)
    {
        t_symbol* absPath;
        char inputConform[MAX_PATH_CHARS];

        if (path_absolutepath(&absPath, atom_getsym(argv), NULL, 0))
        {
            object_error((t_object*)x, "The file “%s” does not exist. ", atom_getsym(argv + i)->s_name);
            return;
        }

        // need that backslash in Windows paths
        path_nameconform(absPath->s_name, &inputConform
            , PATH_STYLE_NATIVE, PATH_TYPE_BOOT);

        CoInitialize(NULL);
        ITEMIDLIST* pidl = ILCreateFromPath(inputConform);
        if (pidl)
        {
            SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
            ILFree(pidl);
        }


    }
    else //multiple files
    {
        int j; int k;
        int fileCounter = 0;
        t_symbol* inputFolder;
        char inputFile[MAX_PATH_CHARS]; // not used

        ITEMIDLIST* selection[1024];
        ITEMIDLIST* dir;


        for (i = 0; i < argc; i++)
        {

            t_symbol* absPath;
            char inputConform[MAX_PATH_CHARS];
            char fileFolder[MAX_PATH_CHARS];

            if (atom_getsym(argv + i) == _sym_nothing)
            {
                continue;
            }

            if (path_absolutepath(&absPath, atom_getsym(argv+i), NULL, 0))
            {
                object_error((t_object*)x, "The file “%s” does not exist. ", atom_getsym(argv + i)->s_name);
                atom_setsym(argv + i, _sym_nothing);
                continue;
            }

            // the file exists
            // need that backslash in Windows paths
            path_nameconform(absPath->s_name, &inputConform, PATH_STYLE_NATIVE, PATH_TYPE_BOOT);

            // get folder
            path_splitnames(inputConform, fileFolder, inputFile);

            //remember current focused folder before checking other files
            inputFolder = gensym(fileFolder);

            dir = ILCreateFromPath(fileFolder); // the file's folder

            selection[fileCounter] = ILCreateFromPath(inputConform);

            //  find other files of the same folder
            for (j = i+1; j < argc; j++)
            {
                if (atom_getsym(argv + j) == _sym_nothing)
                {
                    continue;
                }

                if (path_absolutepath(&absPath, atom_getsym(argv + j), NULL, 0))
                {
                    object_error((t_object*)x, "The file “%s” does not exist. ", atom_getsym(argv + j)->s_name);
                    atom_setsym(argv + j, _sym_nothing);
                    continue;
                }
                path_nameconform(absPath->s_name, &inputConform, PATH_STYLE_NATIVE, PATH_TYPE_BOOT);
                path_splitnames(inputConform, fileFolder, inputFile);

                if (gensym(fileFolder) == inputFolder)
                {
                    //found another file of the same folder in the input list
                    fileCounter++;
                    selection[fileCounter] = ILCreateFromPath(inputConform);
                    atom_setsym(argv + j, _sym_nothing); 
                }
               
            }

            CoInitialize(NULL);
            //Perform 
            SHOpenFolderAndSelectItems(dir, fileCounter + 1, selection, 0);

            
            // free
            ILFree(dir);
            for (k = 0; k < fileCounter + 1; k++)
            {
                ILFree(selection[k]);
            }

            //reset file counter
            fileCounter = 0;
                
        } // end of list iteration
        
        
    } // end of handling multiple files


}


#endif





void showfile_assist(t_showfile* x, void* b, long m, long a, char* s)
{
	if (m == ASSIST_INLET) { //inlet
		sprintf(s, "I am inlet %ld", a);
	}
	else {	// outlet
		sprintf(s, "I am outlet %ld", a);
	}
}

void showfile_free(t_showfile* x)
{
	;
}


void *showfile_new(t_symbol *s, long argc, t_atom *argv)
{
    t_showfile* x = (t_showfile*)object_alloc(showfile_class);
	return (x);
}
