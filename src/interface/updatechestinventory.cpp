/*-------------------------------------------------------------------------------

	BARONY
	File: updatechestinventory.cpp
	Desc: contains updateChestInventory()

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../draw.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../sound.hpp"
#include "../net.hpp"
#include "../player.hpp"
#include "interface.hpp"

Entity* openedChest[MAXPLAYERS] = { nullptr };

void repopulateInvItems(list_t* chestInventory)
{
	int c = 0;

	//Step 1: Clear.
	for ( c = 0; c < kNumChestItemsToDisplay; ++c )
	{
		invitemschest[c] = nullptr;
	}

	node_t* node = nullptr;
	Item* item = nullptr;

	c = 0;

	//Step 2: Loop through inventory till reach part visible in chest GUI and add those items.
	for ( node = chestInventory->first; node != nullptr; node = node->next )
	{
		if ( node->element )
		{
			item = (Item*) node->element;
			if ( item )
			{
				++c;
				if ( c <= chestitemscroll )
				{
					continue;
				}
				invitemschest[c - chestitemscroll - 1] = item;
				if ( c > (kNumChestItemsToDisplay - 1) + chestitemscroll )
				{
					break;
				}
			}
		}
	}
}

int numItemsInChest(const int player)
{
	node_t* node = nullptr;

	list_t* chestInventory = nullptr;
	if ( multiplayer == CLIENT )
	{
		chestInventory = &chestInv;
	}
	else if (openedChest[player]->children.first && openedChest[player]->children.first->element)
	{
		chestInventory = (list_t*)openedChest[player]->children.first->element;
	}

	int i = 0;

	if ( chestInventory )
	{
		for (node = chestInventory->first; node != nullptr; node = node->next)
		{
			++i;
		}
	}

	return i;
}

void warpMouseToSelectedChestSlot()
{
	int x = CHEST_INVENTORY_X + (inventoryoptionChest_bmp->w / 2);
	int y = CHEST_INVENTORY_Y + 16 + (inventoryoptionChest_bmp->h * selectedChestSlot) + (inventoryoptionChest_bmp->h / 2);
	SDL_WarpMouseInWindow(screen, x, y);
}

/*-------------------------------------------------------------------------------

	updateChestInventory

	Processes and draws everything related to chest inventory

-------------------------------------------------------------------------------*/

inline void drawChestSlots(const int player)
{
	if ( !openedChest[player] )
	{
		return;
	}

	SDL_Rect pos;
	Item* item = nullptr;

	int highlightingSlot = -1;

	if (omousex >= CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28))
	{
		pos.x = CHEST_INVENTORY_X + 12;
		pos.w = 0;
		pos.h = 0;

		int currentY = CHEST_INVENTORY_Y + 16;
		for ( int i = 0; i < kNumChestItemsToDisplay; ++i, currentY += inventoryoptionChest_bmp->h )
		{
			if ( omousey >= currentY && omousey < currentY + inventoryoptionChest_bmp->h )
			{
				pos.y = currentY;
				item = openedChest[player]->getItemFromChest(invitemschest[i], false, true);
				if ( item != nullptr )
				{
					free(item); //Only YOU can prevent memleaks!
					drawImage(inventoryoptionChest_bmp, nullptr, &pos); //Highlight the moused-over slot.
					highlightingSlot = i;

					bool grabbedItem = false;

					if ( (mousestatus[SDL_BUTTON_LEFT] || *inputPressed(joyimpulses[INJOY_MENU_USE])) )
					{
						mousestatus[SDL_BUTTON_LEFT] = 0;
						*inputPressed(joyimpulses[INJOY_MENU_USE]) = 0;
						item = openedChest[player]->getItemFromChest(invitemschest[i], false);
						messagePlayer(player, language[374], item->description());
						itemPickup(player, item);
						playSound(35 + rand() % 3, 64);
						grabbedItem = true;
					}
					else if ( mousestatus[SDL_BUTTON_RIGHT] )
					{
						mousestatus[SDL_BUTTON_RIGHT] = 0;
						item = openedChest[player]->getItemFromChest(invitemschest[i], true);
						messagePlayer(player, language[374], item->description());
						itemPickup(player, item); //Grab all of that item from the chest.
						playSound(35 + rand() % 3, 64);
						grabbedItem = true;
					}

					if ( grabbedItem )
					{
						list_t* chestInventory = nullptr;
						if ( multiplayer == CLIENT )
						{
							chestInventory = &chestInv;
						}
						else if ( openedChest[player]->children.first && openedChest[player]->children.first->element )
						{
							chestInventory = (list_t*)openedChest[player]->children.first->element;
						}
						repopulateInvItems(chestInventory); //Have to regenerate, otherwise the following if check will often end up evaluating to false. //Doesn't work. #blamedennis

						item = openedChest[player]->getItemFromChest(invitemschest[i], false, true);
						if ( item )
						{
							free(item);
						}
						else
						{
							//Move cursor if this slot is now empty.
							--highlightingSlot;
							selectedChestSlot = highlightingSlot;
							if ( selectedChestSlot >= 0 )
							{
								warpMouseToSelectedChestSlot();
							}
							else
							{
								warpMouseToSelectedInventorySlot();
							}
						}
					}
				}
			}
		}
	}

	if ( highlightingSlot >= 0 )
	{
		selectedChestSlot = highlightingSlot;
	}
	else
	{
		selectedChestSlot = -1;
	}
}

void updateChestInventory(const int player)
{
	if ( !openedChest[player] )
	{
		for ( int c = 0; c < kNumChestItemsToDisplay; ++c )
		{
			invitemschest[c] = nullptr;
		}
		return;
	}

	SDL_Rect pos;
	node_t* node, *nextnode;
	int y, c;
	int chest_buttonclick = 0;
	Item* item;

	//Chest inventory GUI.

	//Center the chest GUI.
	//pos.x = ((xres - winx) / 2) - (inventory_bmp->w / 2);
	pos.x = CHEST_INVENTORY_X; //(winx + ((winw / 2) - (inventoryChest_bmp->w / 2)))
	//pos.x = character_bmp->w;
	//pos.y = ((yres - winy) / 2) - (inventory_bmp->h / 2);
	pos.y = CHEST_INVENTORY_Y; //(winy + ((winh - winy) - (inventoryChest_bmp->h / 2)));
	drawImage(inventoryChest_bmp, NULL, &pos);

	// buttons
	if ( inputs.bMouseLeft(player) && !inputs.getUIInteraction(player)->selectedItem )
	{
		if (openedChest[player])
		{
			//Chest inventory scroll up button.
			if (omousey >= CHEST_INVENTORY_Y + 16 && omousey < CHEST_INVENTORY_Y + 52)
			{
				if (omousex >= CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28) && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 12))
				{
					chest_buttonclick = 7;
					chestitemscroll--;
					inputs.mouseClearLeft(player);
				}
			}
			//Chest inventory scroll down button.
			else if (omousey >= CHEST_INVENTORY_Y + 52 && omousey < CHEST_INVENTORY_Y + 88)
			{
				if (omousex >= CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28) && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 12))
				{
					chest_buttonclick = 8;
					chestitemscroll++;
					inputs.mouseClearLeft(player);
				}
			}
			else if (omousey >= CHEST_INVENTORY_Y && omousey < CHEST_INVENTORY_Y + 15)
			{
				//Chest inventory close button.
				if (omousex >= CHEST_INVENTORY_X + 393 && omousex < CHEST_INVENTORY_X + 407)
				{
					chest_buttonclick = 9;
					inputs.mouseClearLeft(player);
				}
				//Chest inventory grab all button.
				if (omousex >= CHEST_INVENTORY_X + 376 && omousex < CHEST_INVENTORY_X + 391)
				{
					chest_buttonclick = 10;
					inputs.mouseClearLeft(player);
				}
				if (omousex >= CHEST_INVENTORY_X && omousex < CHEST_INVENTORY_X + 377 && omousey >= CHEST_INVENTORY_Y && omousey < CHEST_INVENTORY_Y + 15)
				{
					gui_clickdrag = true;
					dragging_chestGUI = true;
					dragoffset_x = omousex - CHEST_INVENTORY_X;
					dragoffset_y = omousey - CHEST_INVENTORY_Y;
					inputs.mouseClearLeft(player);
				}
			}
		}
	}

	if ( *inputPressed(joyimpulses[INJOY_MENU_CHEST_GRAB_ALL]) )   //Gamepad "Y" button grabs all items in chest.
	{
		*inputPressed(joyimpulses[INJOY_MENU_CHEST_GRAB_ALL]) = 0;
		chest_buttonclick = 10;
	}

	// mousewheel
	if ( omousex >= CHEST_INVENTORY_X + 12 && omousex < CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28) )
	{
		if ( omousey >= CHEST_INVENTORY_Y + 16 && omousey < CHEST_INVENTORY_Y + (inventoryChest_bmp->h - 8) )
		{
			if ( mousestatus[SDL_BUTTON_WHEELDOWN] )
			{
				mousestatus[SDL_BUTTON_WHEELDOWN] = 0;
				chestitemscroll++;
			}
			else if ( mousestatus[SDL_BUTTON_WHEELUP] )
			{
				mousestatus[SDL_BUTTON_WHEELUP] = 0;
				chestitemscroll--;
			}
		}
	}

	if (dragging_chestGUI)
	{
		if (gui_clickdrag)
		{
			chestgui_offset_x = (omousex - dragoffset_x) - (CHEST_INVENTORY_X - chestgui_offset_x);
			chestgui_offset_y = (omousey - dragoffset_y) - (CHEST_INVENTORY_Y - chestgui_offset_y);
			if (CHEST_INVENTORY_X <= 0)
			{
				chestgui_offset_x = 0 - (CHEST_INVENTORY_X - chestgui_offset_x);
			}
			if (CHEST_INVENTORY_X > 0 + xres - inventoryChest_bmp->w)
			{
				chestgui_offset_x = (0 + xres - inventoryChest_bmp->w) - (CHEST_INVENTORY_X - chestgui_offset_x);
			}
			if (CHEST_INVENTORY_Y <= 0)
			{
				chestgui_offset_y = 0 - (CHEST_INVENTORY_Y - chestgui_offset_y);
			}
			if (CHEST_INVENTORY_Y > 0 + yres - inventoryChest_bmp->h)
			{
				chestgui_offset_y = (0 + yres - inventoryChest_bmp->h) - (CHEST_INVENTORY_Y - chestgui_offset_y);
			}
		}
		else
		{
			dragging_chestGUI = false;
		}
	}

	list_t* chest_inventory = NULL;
	if ( multiplayer == CLIENT )
	{
		chest_inventory = &chestInv;
	}
	else if (openedChest[player]->children.first && openedChest[player]->children.first->element)
	{
		chest_inventory = (list_t*)openedChest[player]->children.first->element;
	}

	if (!chest_inventory)
	{
		messagePlayer(0, "Warning: openedChest[%d] has no inventory. This should not happen.", player);
	}
	else
	{
		//Print the window label signifying this as the chest inventory GUI.
		ttfPrintText(ttf8, (CHEST_INVENTORY_X + 2 + ((inventoryChest_bmp->w / 2) - ((TTF8_WIDTH * 15) / 2))), CHEST_INVENTORY_Y + 4, language[373]);

		//Chest inventory up button.
		if (chest_buttonclick == 7)
		{
			pos.x = CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28);
			pos.y = CHEST_INVENTORY_Y + 16;
			pos.w = 0;
			pos.h = 0;
			drawImage(invup_bmp, NULL, &pos);
		}
		//Chest inventory down button.
		if (chest_buttonclick == 8)
		{
			pos.x = CHEST_INVENTORY_X + (inventoryChest_bmp->w - 28);
			pos.y = CHEST_INVENTORY_Y + 52;
			pos.w = 0;
			pos.h = 0;
			drawImage(invdown_bmp, NULL, &pos);
		}
		//Chest inventory close button.
		if (chest_buttonclick == 9)
		{
			pos.x = CHEST_INVENTORY_X + 393;
			pos.y = CHEST_INVENTORY_Y;
			pos.w = 0;
			pos.h = 0;
			drawImage(invclose_bmp, NULL, &pos);
			if (openedChest[player])
			{
				openedChest[player]->closeChest();
				return; //Crashes otherwise, because the rest of this function runs without a chest...
			}
		}
		//Chest inventory grab all items button.
		if (chest_buttonclick == 10)
		{
			pos.x = CHEST_INVENTORY_X + 376;
			pos.y = CHEST_INVENTORY_Y;
			pos.w = 0;
			pos.h = 0;
			drawImage(invgraball_bmp, NULL, &pos);
			for (node = chest_inventory->first; node != NULL; node = nextnode)
			{
				nextnode = node->next;
				if (node->element && openedChest[player])
				{
					item = openedChest[player]->getItemFromChest(static_cast<Item* >(node->element), true);
					if ( item != NULL )
					{
						messagePlayer(player, language[374], item->description());
						itemPickup(player, item);
						playSound(35 + rand() % 3, 64);
					}
				}
			}
		}

		drawChestSlots(player);

		//Okay, now prepare to render all the items.
		y = CHEST_INVENTORY_Y + 22;
		c = 0;
		if (chest_inventory)
		{
			c = numItemsInChest(player);
			chestitemscroll = std::max(0, std::min(chestitemscroll, c - 4));

			repopulateInvItems(chest_inventory);

			c = 0;

			//Actually render the items.
			for (node = chest_inventory->first; node != NULL; node = node->next)
			{
				if (node->element)
				{
					item = (Item*) node->element;
					if (item)
					{
						c++;
						if (c <= chestitemscroll)
						{
							continue;
						}
						char tempstr[64] = { 0 };
						strncpy(tempstr, item->description(), 46);
						if ( strlen(tempstr) == 46 )
						{
							strcat(tempstr, " ...");
						}
						ttfPrintText(ttf8, CHEST_INVENTORY_X + 36, y, tempstr);
						pos.x = CHEST_INVENTORY_X + 16;
						pos.y = CHEST_INVENTORY_Y + 17 + 18 * (c - chestitemscroll - 1);
						pos.w = 16;
						pos.h = 16;
						drawImageScaled(itemSprite(item), NULL, &pos);
						y += 18;
						if (c > (kNumChestItemsToDisplay - 1) + chestitemscroll)
						{
							break;
						}
					}
				}
			}
		}
	}
}

void selectChestSlot(const int player, const int slot)
{
	//TODO?: Grab amount (difference between slot and selectedChestSlot)?

	if ( slot < selectedChestSlot )
	{
		//Moving up.

		/*
		 * Possible cases:
		 * * 1) Move cursor up the GUI through different selectedChestSlot.
		 * * 2) Page up through chestitemscroll--
		 * * 3) Scrolling up past top of chest, no chestitemscroll (move back to inventory)
		 */

		if ( selectedChestSlot <= 0 )
		{
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very top of chest inventory, can't go any further. Return to inventory.
			 * * B) Page up, scrolling through chestitemscroll.
			 */

			if ( chestitemscroll <= 0 )
			{
				//Case 3/A: Return to inventory.
				selectedChestSlot = -1;
			}
			else
			{
				//Case 2/B: Page up through chest inventory.
				--chestitemscroll;
			}
		}
		else
		{
			//Covers case 1.

			//Move cursor up the GUI through different selectedChestSlot (--selectedChestSlot).
			--selectedChestSlot;
			warpMouseToSelectedChestSlot();
		}
	}
	else if ( slot > selectedChestSlot )
	{
		//Moving down.

		/*
		 * Possible cases:
		 * * 1) Moving cursor down through GUI through different selectedChestSlot.
		 * * 2) Scrolling down past bottom of chest through chestitemscroll++
		 * * 3) Scrolling down past bottom of chest, max chest scroll (revoke move -- can't go beyond limit of chest).
		 */

		Item* item = nullptr;

		if ( selectedChestSlot >= (kNumChestItemsToDisplay - 1) )
		{
			//Covers cases 2 & 3.

			/*
			 * Possible cases:
			 * * A) Hit very bottom of chest inventory, can't even scroll any further! Revoke movement.
			 * * B) Page down, scrolling through chestitemscroll.
			 */

			++chestitemscroll; //chestitemscroll is sanitized in updateChestInventory().
		}
		else
		{
			//Covers case 1.
			//Move cursor down through the GUi through different selectedChestSlot (++selectedChestSlot).
			//This is a little bit trickier since must revoke movement if there is no item in the next slot!

			/*
			 * Two possible cases:
			 * * A) Items below this. Advance selectedChestSlot to them.
			 * * B) On last item already. Do nothing (revoke movement).
			 */

			item = openedChest[player]->getItemFromChest(invitemschest[selectedChestSlot + 1], false, true);


			if ( item )
			{
				free(item); //Cleanup duty.

				++selectedChestSlot;
				warpMouseToSelectedChestSlot();
			}
			else
			{
			}
		}
	}
}
