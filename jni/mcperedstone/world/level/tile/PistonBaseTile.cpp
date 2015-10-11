#include "PistonBaseTile.h"
#include "mcpe/world/material/Material.h"
#include "mcpe/world/level/TileSource.h"
#include "mcpe/world/Facing.h"
#include "mcpe/world/item/Item.h"
#include "mcpe/world/item/ItemInstance.h"
#include "mcpe/world/phys/AABB.h"
#include "mcpe/world/entity/Mob.h"
#include <cmath>
#include <algorithm>


PistonBaseTile::PistonBaseTile(int blockId, bool sticky) : Tile(blockId, &Material::stone) {
	init();

	if(!sticky) {
		setNameId("pistonBase");
		tex = getTextureUVCoordinateSet("piston_top_normal", 0);
	}
	else {
		setNameId("pistonStickyBase");
		tex = getTextureUVCoordinateSet("piston_top_sticky", 0);
	}

	this->sticky = sticky;
	creativeTab = CreativeTab::ITEMS;
	renderType = 0;
	setDestroyTime(0.5F);
	solid[blockId] = false;
	lightBlock[blockId] = 0;

	texture_inner = getTextureUVCoordinateSet("piston_inner", 0);
	texture_bottom = getTextureUVCoordinateSet("piston_bottom", 0);
	texture_side = getTextureUVCoordinateSet("piston_side", 0);
}

/*
	Get the textures for the TileItem
*/
const TextureUVCoordinateSet& PistonBaseTile::getTexture(signed char side, int data) {
	return (side == 1)? tex : texture_side;
}

/*
	Get the textures for the block in the world
*/
const TextureUVCoordinateSet& PistonBaseTile::getTexture(TileSource* region, int x, int y, int z, signed char side) {
	int data = region->getData(x, y, z);
	bool powered = isPowered(data);
	int rotation = getRotation(data);

	if(side == rotation)
		return (powered)? texture_inner : tex;
	int opposite[6] = { 1, 0, 3, 2, 5, 4 };
	if(side == opposite[rotation])
		return texture_bottom;

	return texture_side;
}

/*
	Get the shape for the inventory block
*/

const AABB& PistonBaseTile::getVisualShape(unsigned char data, AABB& shape, bool idk) {
	shape.set(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
	return shape;
}

const AABB& PistonBaseTile::getVisualShape(TileSource* region, int x, int y, int z, AABB& shape, bool idk) {
	if(isPowered(region->getData(x, y, z))) {
		switch(getRotation(region->getData(x, y, z))) {
		case 0:
			shape.set(0.0F, 0.25F, 0.0F, 1.0F, 1.0F, 1.0F);
			break;
		case 1:
			shape.set(0.0F, 0.0F, 0.0F, 1.0F, 0.75F, 1.0F);
			break;
		case 2:
			shape.set(0.0F, 0.0F, 0.25F, 1.0F, 1.0F, 1.0F);
			break;
		case 3:
			shape.set(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.75F);
			break;
		case 4:
			shape.set(0.25F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
			break;
		case 5:
			shape.set(0.0F, 0.0F, 0.0F, 0.75F, 1.0F, 1.0F);
			break;
		}
	}
	else
		shape.set(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);

	return shape;
}

int PistonBaseTile::getRotation(int data) {
	return data & 7;
}

bool PistonBaseTile::isPowered(int data) {
	return (data & 8) != 0;
}

void PistonBaseTile::neighborChanged(TileSource* region, int x, int y, int z, int neighborX, int neighborY, int neighborZ) {
	updateState(region, x, y, z);
}

void PistonBaseTile::onPlace(TileSource* region, int x, int y, int z) {
	updateState(region, x, y, z);
}

void PistonBaseTile::updateState(TileSource* region, int x, int y, int z) {
	int data = region->getData(x, y, z);
	int rotation = getRotation(data);
	if(data == 7)
		return;
	bool shouldBePowered = hasPower(region, x, y, z, rotation);
	if(shouldBePowered && !isPowered(data)) {
		if(canPushRow(region, x, y, z, rotation)) {
			region->setTileAndData(x, y, z, {id, rotation | 8}, 0);
			region->tileEvent(x, y, z, 0, rotation);
		}
	}
	else if(!shouldBePowered && isPowered(data)) {
		region->setTileAndData(x, y, z, {id, rotation}, 0);
		region->tileEvent(x, y, z, 1, rotation);
	}
}

bool PistonBaseTile::hasPower(TileSource* region, int x, int y, int z, int rotation) {
	// This can only return true if it is not being powered at its face
	// This is because back in Minecraft beta 1.7 (when pistons were first added), the
	// game crashes when we powered a piston at its face, because it would become powered
	// and unpowered in the same tick.
	if(rotation != 0 && region->getIndirectPowerOutput(x, y - 1, z, 0))
		return true;
	if(rotation != 1 && region->getIndirectPowerOutput(x, y + 1, z, 1))
		return true;
	if(rotation != 2 && region->getIndirectPowerOutput(x, y, z - 1, 2))
		return true;
	if(rotation != 3 && region->getIndirectPowerOutput(x, y, z + 1, 3))
		return true;
	if(rotation != 4 && region->getIndirectPowerOutput(x - 1, y, z, 4))
		return true;
	if(rotation != 5 && region->getIndirectPowerOutput(x + 1, y, z, 5))
		return true;

	// So BUD switches work
	if(region->getIndirectPowerOutput(x, y, z, 0) ||
		region->getIndirectPowerOutput(x, y + 2, z, 1) ||
		region->getIndirectPowerOutput(x, y + 1, z - 1, 2) ||
		region->getIndirectPowerOutput(x, y + 1, z + 1, 3) ||
		region->getIndirectPowerOutput(x - 1, y + 1, z, 4) ||
		region->getIndirectPowerOutput(x + 1, y + 1, z, 5))
			return true;
}

int PistonBaseTile::getPlacementDataValue(Mob* player, int x, int y, int z, signed char side, float xx, float yy, float zz, int data) {
	if(abs(player->x - x) < 2.0F && abs(player->z - z) < 2.0F) {
		float temp = player->y + 1.82F - player->heightOffset;
		if(temp - y > 2.0F)
			return 1;
		if(y - temp > 0.0F)
			return 0;
	}
	int temp2 = (int) floor(((player->yaw * 4.0F) / 360.0F) + 0.5F) & 3;
	return temp2 == 0 ? 2 : (temp2 == 1 ? 5 : (temp2 == 2 ? 3 : (temp2 == 3 ? 4 : 0)));
}

bool PistonBaseTile::canPushRow(TileSource* region, int x, int y, int z, int rotation) {
	int xx = x + Facing::STEP_X[rotation];
	int yy = y + Facing::STEP_Y[rotation];
	int zz = z + Facing::STEP_Z[rotation];
	int counter = 0;
	do {
		if(yy <= 0 || yy >= 127) break; // Can't push blocks out of the world
		PistonPushInfo pushInfo = getPushInfoFor(region, xx, yy, zz);
		if(pushInfo == PistonPushInfo::NO_PUSH)
			return false;
		if(pushInfo == PistonPushInfo::REPLACE)
			break;
		if(pushInfo == PistonPushInfo::REPLACE_DROP)
			break;
			
		xx += Facing::STEP_X[rotation];
		yy += Facing::STEP_Y[rotation];
		zz += Facing::STEP_Z[rotation];
		counter++;
	} while(counter < 13); // You may only push 12 blocks

	return true;
}

void PistonBaseTile::triggerEvent(TileSource* region, int x, int y, int z, int eventType, int rotation) {
	//this->ignoreUpdates = true;
	if(eventType == 0) {
		if(actuallyPushRow(region, x, y, z, rotation)) {
			region->setTileAndData(x, y, z, {id, rotation | 8}, 3);
			// play sound
		} else
			region->setTileAndData(x, y, z, {id, rotation}, 3);
	} else if(eventType == 1) {
		if(sticky) {
			int pullX = x + Facing::STEP_X[rotation] * 2;
			int pullY = y + Facing::STEP_Y[rotation] * 2;
			int pullZ = z + Facing::STEP_Z[rotation] * 2;
			int pullID = region->getTile(pullX, pullY, pullZ).id;
			int pullData = region->getData(pullX, pullY, pullZ);

			if(Tile::tiles[pullID] != NULL) {
				PistonPushInfo pushInfo = getPushInfoFor(region, pullX, pullY, pullZ);
				if(pushInfo == PistonPushInfo::MAY_PUSH) {
					region->setTileAndData(pullX - Facing::STEP_X[rotation], pullY - Facing::STEP_Y[rotation], pullZ - Facing::STEP_Z[rotation], {pullID, pullData}, 3);
					region->setTileAndData(pullX, pullY, pullZ, {0, 0}, 3);
				}
				else
					region->setTileAndData(pullX - Facing::STEP_X[rotation], pullY - Facing::STEP_Y[rotation], pullZ - Facing::STEP_Z[rotation], {0, 0}, 3);
			}
			else
				region->setTileAndData(pullX - Facing::STEP_X[rotation], pullY - Facing::STEP_Y[rotation], pullZ - Facing::STEP_Z[rotation], {0, 0}, 3);
		} else {
			//this->ignoreUpdates = false;
			region->setTileAndData(x + Facing::STEP_X[rotation], y + Facing::STEP_Y[rotation], z + Facing::STEP_Z[rotation], {0, 0}, 3);
			//this->ignoreUpdates = true;
		}
	}
	//this->ignoreUpdates = false;
}

void PistonBaseTile::pushEntitiesInto(TileSource* region, int x, int y, int z, int xx, int yy, int zz) {
	AABB bb({x, y, z}, {x + 1, y + 1, z + 1});
	EntityList& list = region->getEntities(NULL, bb);
	for(Entity* e : list) {
		e->move(xx, yy, zz);
	}
}

bool PistonBaseTile::actuallyPushRow(TileSource* region, int x, int y, int z, int rotation) {
	int xx = x + Facing::STEP_X[rotation];
	int yy = y + Facing::STEP_Y[rotation];
	int zz = z + Facing::STEP_Z[rotation];
	int counter = 0;
	do {
		if(yy <= 0 || yy >= 127)
			break;
		PistonPushInfo pushInfo = getPushInfoFor(region, xx, yy, zz);
		if(pushInfo == PistonPushInfo::NO_PUSH)
			return false;
		if(pushInfo == PistonPushInfo::REPLACE)
			break;
		if(pushInfo == PistonPushInfo::REPLACE_DROP) {
			Tile* toDrop = region->getTilePtr(xx, yy, zz);
			if(toDrop) {
				Item* resource = Item::items[toDrop->getResource(NULL, 0, 0)];
				toDrop->popResource(region, xx, yy, zz, ItemInstance(resource, 1, 0));
			}
			break;
		}
		
		if(counter == 12)
			return false;
			
		xx += Facing::STEP_X[rotation];
		yy += Facing::STEP_Y[rotation];
		zz += Facing::STEP_Z[rotation];
		counter++;
	} while(counter < 13);
	
	pushEntitiesInto(region, xx, yy, zz, Facing::STEP_X[rotation], Facing::STEP_Y[rotation], Facing::STEP_Z[rotation]);

	while(xx != x || yy != y || zz != z) {
		int i2 = xx - Facing::STEP_X[rotation];
		int k2 = yy - Facing::STEP_Y[rotation];
		int l2 = zz - Facing::STEP_Z[rotation];
		xx = i2;
		yy = k2;
		zz = l2;
		int pushID = region->getTile(xx, yy, zz).id;
		int pushData = region->getData(xx, yy, zz);
		
		PistonPushInfo pushInfo = getPushInfoFor(region, xx, yy + 1, zz);
		if(pushInfo == PistonPushInfo::REPLACE_DROP) {
			Tile* toDrop = region->getTilePtr(xx, yy + 1, zz);
			if(toDrop) {
				Item* resource = Item::items[toDrop->getResource(NULL, 0, 0)];
				region->setTileAndData(xx, yy + 1, zz, {0, 0}, 2);
				toDrop->popResource(region, xx, yy + 1, zz, ItemInstance(resource, 1, 0));
			}
		}
		
		if(pushID == id && i2 == x && k2 == y && l2 == z) {
			region->setTileAndData(xx + Facing::STEP_X[rotation], yy + Facing::STEP_Y[rotation], zz + Facing::STEP_Z[rotation], {34, rotation | (sticky? 8 : 0)}, 3);
		} else {
			region->setTileAndData(xx + Facing::STEP_X[rotation], yy + Facing::STEP_Y[rotation], zz + Facing::STEP_Z[rotation], {pushID, pushData}, 3);
		}
	}
	//ts->setTileAndData(xx + Facing::xSide[rot], yy + Facing::ySide[rot], zz + Facing::zSide[rot], 34, rot, 0);
	return true;
}

PistonPushInfo PistonBaseTile::getPushInfoFor(TileSource* region, int x, int y, int z) {
	Tile* tile = region->getTilePtr(x, y, z);
	if(tile == Tile::obsidian || tile == Tile::unbreakable || tile == Tile::pistonArm || tile == Tile::portal)
		return PistonPushInfo::NO_PUSH;
	if(tile == Tile::pistonNormal || tile == Tile::pistonSticky)
		if(isPowered(region->getData(x, y, z)))
			return PistonPushInfo::NO_PUSH;
	if(region->getTileEntity({x, y, z}))
		return PistonPushInfo::NO_PUSH;
	
	if(tile == NULL || tile == Tile::water || tile == Tile::calmWater || tile == lava || tile == Tile::calmLava || tile == Tile::fire || tile->renderType == 19 || tile->renderType == 20)
		return PistonPushInfo::REPLACE;
	
	if(tile) {
		bool shouldDrop = false;
		
		switch(tile->renderType) {
		case 1:
			if(tile != Tile::web)
				shouldDrop = true;
			break;
		case 2:
		case 5:
		case 7:
		case 8:
		case 12:
		case 14:
		case 15:
		case 23:
		case 28:
		case 40:
		case 65:
		case 66:
			shouldDrop = true;
		}
		if(shouldDrop)
			return PistonPushInfo::REPLACE_DROP;
	}
	
	return PistonPushInfo::MAY_PUSH;
}
