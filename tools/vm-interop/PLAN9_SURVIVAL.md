# Plan 9 Survival Guide for Linux Users

## Quick Editor Escapes
- **sam**: Type `q` + Enter in the command window to quit
- **acme**: Right-click on "Del" in the window tag to close
- **rio**: Right-click on window border, select "Delete"

## Use `ed` - The Simple Line Editor (Easiest!)
```rc
# Edit a file
ed filename.txt
a                  # append mode
Type your text here
.                  # dot on its own line to stop
w                  # write file
q                  # quit

# Quick edit existing file
ed file.txt
1,$p              # print all lines
2d                # delete line 2
3c                # change line 3
new text here
.
w                 # save
q                 # quit
```

## Create Files Without Editors
```rc
# Create file with echo
echo 'Hello World' > file.txt

# Append to file
echo 'More text' >> file.txt

# Multi-line with heredoc-style
{
echo 'line 1'
echo 'line 2'
echo 'line 3'
} > myfile.rc

# Copy from console input
cat > newfile.txt
Type your content
Press Ctrl+D when done
```

## Mouse Issues Workaround
```rc
# Use keyboard navigation in rio
# ESC key often works as middle-click
# Shift+Click sometimes works as right-click

# For acme without good mouse:
# Just use ed or echo commands instead!
```

## Essential Commands for Getting Work Done

### View files
```rc
cat filename       # display file
sed 5q file        # show first 5 lines (like head)
sed -n '10,20p'    # show lines 10-20
```

### Edit files without editors
```rc
# Replace text in file
sed 's/old/new/g' file.txt > file.new && mv file.new file.txt

# Delete lines
sed '/pattern/d' file.txt > file.new && mv file.new file.txt

# Insert line at beginning
{echo 'new first line'; cat file.txt} > file.new && mv file.new file.txt
```

### Quick Navigation
```rc
cd /               # go to root
cd                 # go home
pwd                # where am I?
ls -l              # list files
du -a | grep txt   # find all txt files
```

## Cheat: Use Remote Editing!

Since you have the VM-Interop setup, edit files on Linux and access them from Plan 9:

1. On Linux:
```bash
# Edit files in /home/scott/Repo/VM-Interop/
vim /home/scott/Repo/VM-Interop/mycode.c
```

2. In Plan 9:
```rc
# Access the file
cat /n/interop/mycode.c
cp /n/interop/mycode.c /tmp/mycode.c
```

## Emergency Exit Commands
```
Del       # Delete key often closes things
q + Enter # Quit in sam
Ctrl+D    # EOF, exits many programs
```

## Pro Tip: Just Use Shell Commands
Instead of fighting with editors, use rc shell for everything:
- Create files with echo/cat
- Edit with sed
- View with cat
- Search with grep

This way you can actually get work done!