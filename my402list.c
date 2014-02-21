#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "my402list.h"

int My402ListLength (My402List *list)
{
	return list->num_members;
}

int My402ListEmpty (My402List *list)
{
	if(list->num_members==0)
		return TRUE;
	else
		return FALSE;
}

int My402ListAppend (My402List *list, void *obj)
{
	My402ListElem *temp = (My402ListElem *) malloc (sizeof(My402ListElem));
	
	if(temp == NULL)
		return FALSE;
	else
	{
		My402ListElem *last_elem = My402ListLast(list);
		temp->obj=obj;
		temp->next=&(list->anchor);
		temp->prev=last_elem;

		last_elem->next=temp;

		list->anchor.prev=temp;
		list->num_members++;

	//	printf("No. of items in the list: %d\n", list->num_members);
		return TRUE;
	}
}



int My402ListPrepend (My402List *list, void *obj)
{
	My402ListElem *temp = (My402ListElem *) malloc(sizeof(My402ListElem));
	if(temp == NULL)
		return FALSE;
	else
	{
		My402ListElem *first_elem = My402ListFirst(list);
		temp->obj=obj;
		temp->next=first_elem;
		temp->prev=&(list->anchor);

		first_elem->prev=temp;

		list->anchor.next=temp;
		list->num_members++;

		return TRUE;
	}
}

void My402ListUnlink (My402List *list, My402ListElem *elem)
{
		elem->prev->next = elem->next;
		elem->next->prev = elem->prev;
		
		list->num_members--;
		free(elem);
}

void My402ListUnlinkAll (My402List *list)
{
	My402ListElem *temp1, *temp2;

	for(temp1=My402ListFirst(list),temp2=temp1; temp1!=NULL; temp1=My402ListNext(list,temp2))
		{
			temp2=temp1;
			free(temp1);
		}

		list->num_members = 0;
		
}

int My402ListInsertBefore (My402List *list, void *obj, My402ListElem *elem)
{
	My402ListElem *temp = (My402ListElem *) malloc(sizeof(My402ListElem));

	if(elem == NULL)
		return My402ListPrepend(list, obj);

	if(temp==NULL)
		return FALSE;

	temp->next=elem;
	temp->prev=elem->prev;
	temp->obj=obj;

	elem->prev=temp;
	temp->prev->next=temp;

	list->num_members++;

	return TRUE;

}

int My402ListInsertAfter (My402List *list, void *obj, My402ListElem *elem)
{
	My402ListElem *temp = (My402ListElem *) malloc(sizeof(My402ListElem));

	if(elem == NULL)
		return My402ListAppend(list, obj);

	if(temp==NULL)
		return FALSE;

	temp->next=elem->next;
	temp->prev=elem;
	temp->obj=obj;

	elem->next=temp;
	temp->next->prev=temp;

	list->num_members++;

	return TRUE;
}

My402ListElem *My402ListFirst(My402List *list)
{
	if(list!=NULL)
		return list->anchor.next;
	else
		return NULL;
}

My402ListElem *My402ListFind(My402List *list, void *obj)
{
	My402ListElem *temp;

	for(temp=My402ListFirst(list);temp!=NULL;temp=My402ListNext(list,temp))
	{
		if(temp->obj == obj)
			break;
	}
return temp;
}

My402ListElem *My402ListNext(My402List *list, My402ListElem *elem)
{
	if(elem->next == &(list->anchor))
		return NULL;
	else
		return elem->next;
}

My402ListElem *My402ListPrev(My402List *list, My402ListElem *elem)
{
	if(elem->prev == &(list->anchor))
		return NULL;
	else
		return elem->prev;
}

My402ListElem *My402ListLast(My402List *list)
{
	if(list!=NULL)
		return list->anchor.prev;
	else
		return NULL;
}

int My402ListInit(My402List *list)
{
	if(list!=NULL)
	{
		list->num_members=0;
		list->anchor.next=&(list->anchor);
		list->anchor.prev=&(list->anchor);
	/*	list->Length=My402ListLength(list);
		list->Empty=My402ListEmpty();
		list->Append=My402ListAppend();
		list->Prepend=My402ListPrepend();
		list->Unlink=My402ListUnlink();
		list->UnlinkAll=My402ListUnlinkAll();
		list->InsertBefore=My402ListInsertBefore();
		list->InsertAfter=My402ListInsertAfter();
		list->First=My402ListFirst();
		list->Last=My402ListLast();
		list->Prev=My402ListPrev();
		list->Next=My402ListNext();
		list->Find=My402ListFind(); */
		return TRUE;
	}

	return FALSE;
}


