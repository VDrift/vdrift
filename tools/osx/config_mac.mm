#import <Cocoa/Cocoa.h>

char* get_mac_data_dir()
{
    NSAutoreleasePool	*pool = [[NSAutoreleasePool alloc] init];
	NSString * path = [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] stringByAppendingString:@"/data"];

	CFStringRef resolvedPath = nil;
	CFURLRef url = CFURLCreateWithFileSystemPath(NULL, (CFStringRef)path, kCFURLPOSIXPathStyle, true);
	
	if (url != NULL)
	{
		FSRef fsRef;
		
		if (CFURLGetFSRef(url, &fsRef))
		{
			Boolean isFolder, isAlias;
			OSErr oserr = FSResolveAliasFile (&fsRef, true, &isFolder, &isAlias);
			
			if(oserr != noErr)
			{
				NSLog(@"FSResolveAliasFile failed: status = %d", oserr);
			}
			else
			{
				if(isAlias)
				{
					CFURLRef resolved_url = CFURLCreateFromFSRef(NULL, &fsRef);
					
					if (resolved_url != NULL)
					{
						resolvedPath = CFURLCopyFileSystemPath(resolved_url, kCFURLPOSIXPathStyle);
						CFRelease(resolved_url);
					}
				}
			}
		}
		else // Failed to convert URL to a file or directory object.
		{
			NSApplication *myApplication;
			myApplication = [NSApplication sharedApplication];
			
			NSAlert *theAlert = [NSAlert alertWithMessageText: @"Can't find data folder"
								defaultButton: @"OK"
								alternateButton: nil
								otherButton: nil
                                informativeTextWithFormat: @"Please make sure vdrift.app is in the same folder as the \"data\" folder or an alias to the data folder!"];
			[theAlert runModal];
			
			[pool release];
			exit(1);
		}
	}
	
	if(resolvedPath != nil)
	{	
		path = [NSString stringWithString:(NSString *)resolvedPath];
		CFRelease(resolvedPath);
	}
	
	if ([path canBeConvertedToEncoding:[NSString defaultCStringEncoding]])
	{
		char* cpath = (char*) malloc([path cStringLength] + 2);
		
		[path getCString:cpath];
		cpath[[path cStringLength]] = '\0';
		
		[pool release];
		
		return cpath;
	}
	else
	{
		NSApplication *myApplication;
		myApplication = [NSApplication sharedApplication];
		
		NSAlert *theAlert = [NSAlert alertWithMessageText: @"Can't find data folder"
							defaultButton: @"OK"
							alternateButton: nil
							otherButton: nil
							informativeTextWithFormat: @"Please move vdrift to a sane location on your harddisk, without weird characters in it's path!"];
							
		[theAlert runModal];
		
		[pool release];
		exit(1);
	}
}