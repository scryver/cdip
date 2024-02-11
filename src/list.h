
typedef struct LinkItem
{
    struct LinkItem *next;
} LinkItem;

typedef struct LinkList
{
    LinkItem *first;
    LinkItem *last;
} LinkList;

typedef struct DoubleLink
{
    struct DoubleLink *next;
    struct DoubleLink *prev;
} DoubleLink;

func void
initialize_link_list(LinkList *list)
{
    list->first = list->last = 0;
}

func b32
link_list_empty(LinkList *list)
{
    b32 result = list->first == 0;
    return result;
}

func void
link_list_add_end(LinkList *list, LinkItem *item)
{
    item->next = 0;
    list->last = ((list->last ? list->last->next : list->first) = item);
}

func LinkItem *
link_list_pop_first(LinkList *list)
{
    LinkItem *result = list->first;
    if (result) {
        list->first = result->next;
        if (list->first == 0) {
            list->last = 0;
        }
        result->next = 0;
    }
    return result;
}

func void
initialize_double_link(DoubleLink *list)
{
    list->next = list->prev = list;
}

func b32
double_link_empty(DoubleLink *list)
{
    b32 result = list->next == list;
    return result;
}

func void
double_link_add_end(DoubleLink *list, DoubleLink *item)
{
    item->next = list;
    item->prev = list->prev;
    list->prev = item;
    item->prev->next = item;
}

func void
double_link_add_start(DoubleLink *list, DoubleLink *item)
{
    item->prev = list;
    item->next = list->next;
    list->next = item;
    item->next->prev = item;
}

func void
double_link_remove(DoubleLink *item)
{
    item->prev->next = item->next;
    item->next->prev = item->prev;
}
