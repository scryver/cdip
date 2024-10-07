global List *gFreeLists;

func List *append(List *list, void *x)
{
    List *newList = gFreeLists;
    if (newList) {
        gFreeLists = gFreeLists->link;
    } else {
        newList = create(List, ArenaType_Perm);
    }

    if (list) {
        newList->link = list->link;
        list->link = newList;
    } else {
        newList->link = newList;
    }
    newList->x = x;
    return newList;
}

func sze list_length(List *list)
{
    sze result = 0;
    if (list)
    {
        List *sentinel = list;
        do {
            ++result;
            list = list->link;
        } while (list != sentinel);
    }
    return result;
}

func void *array_from_list(List **list, ArenaType arena)
{
    void **array = create_array(list_length(*list) + 1, sizeof(void *), arena);
    sze index = 0;
    if (*list)
    {
        List loop = *list;
        do {
            loop = loop->link;
            array[index++] = loop->x;
        } while (loop != *list);
        loop = (*list)->link;
        (*list)->link = gFreeLists;
        gFreeLists = loop;
    }
    *list = 0;
    array[index] = 0;
    return array;
}
