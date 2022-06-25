# How to use Vim and other tips

Copy the .vimrc file into your home directory on the server using Unix's **scp command**.

Remember to change MATRICOLA with your Unipd Student ID.

Login using your SSH credentials.

    scp -O ./.vimrc MATRICOLA@88.80.187.84:/home/MATRICOLA/.vimrc 

## Content of .vimrc
This .vimrc config file allow you to:
- **Use your mouse**: scrolling wheel to move up or down, click and point
- **Move line Up/Down** using Ctrl+Shift+Up/Down
- Press F8 to **Compile** the program **without exiting Vim**
- Press F9 to **Execute** the program **without exiting Vim**

## Other configurations:
<details>
<summary>Click to expand!</summary>

- Replace tabs with 3 spaces
- Highlight matching parantheses
- Auto indent on brackets
- Show line number
- Highlight current line
- Search characters as they are entered
- Search is case insesitive if no case letters are entered, but case sensitive if case letters are entered
- Highlight search results
</details>


# How to search in VIM:
<details>
<summary>Click to expand!</summary>

Search is **UNIDIRECTIONAL** but when the search reach one end of the file, pressing **n** continue the search starting from the other end of the file.

## Search from the current line **forward** using **/**

x es:

    ESC (to exit Insert mode)

    /query

    ENTER (to stop writing in the search query)

    (now all search results of the query are highlighted)

    n (to move to next occurence of search result)

    ESC (to exit Search mode)

## Search from the current line **backwards** using **?**

x es:

    ESC (to exit Insert mode)

    ?query

    ENTER (to stop writing in the search query)

    (now all search results of the query are highlighted)

    n (to move to next occurence of search result)

    ESC (to exit Search mode)
</details>


# How to Compile and Execute without exiting VIM:
<details>
<summary>Click to expand!</summary>

## Compile

    ESC (to exit Insert mode)

    F8 (compile shortcut)

    CTRL+C (to exit compilation) 

    Enter (to re-enter in vim)

## Execute
    ESC (to exit Insert mode)

    F9 (execute shortcut)

    CTRL+C (to stop the executable) 

    Enter (to re-enter in vim)
</details>



# How to move current line Up or Down in VIM:
<details>
<summary>Click to expand!</summary>

    ESC (to exit Insert mode)

    CTRL+SHIFT+PAGE UP/DOWN (to move line up/down)

    i (to re-enter Insert mode)
</details>


# How to select, copy/cut and paste in VIM:
<details>
<summary>Click to expand!</summary>

    Select with the mouse the text you want to copy

    y (to copy/yank)
    [ALTERNATIVE d (to cut)]

    p (to paste after the cursor)
</details>


# If you've made an error, CTRL+z is u:
<details>
<summary>Click to expand!</summary>
    
    ESC (to exit Insert mode)

    u (to Undo)
</details>


# If you've pressed CTRL + s and now the screen is frozen, press CTRL + q (to unfreeze screen)
<details>
<summary>Click to expand!</summary>

    CTRL + s (now screen is frozen)

    (every command that you type when the screen is frozen will be executed, it just won't be displayed in the terminal)

    CTRL + q (to unfreeze the screen)
</details>