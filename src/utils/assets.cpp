#include <string.h>

#include "platform/platform.h"
#include "utils/memory.h"

#include "assets.h"

static LoadedFile load_file(PlatformAPI *api, MemoryArena *arena, char *filename)
{
  LoadedFile result = {};
  int32_t file_size = api->get_file_size(filename);
  if (PAPI_ERROR(file_size)) {
    printf("Failed to get size of file %s\n", filename);
    return result;
  }

  void *memory = arena->allocate(file_size);
  ASSERT(memory);

  if (PAPI_OK(api->read_file_contents(filename, memory, file_size))) {
    printf("Read file %s of %d bytes\n", filename, file_size);
  } else {
    printf("Failed to read file %s\n", filename);
    return result;
  }

  result.contents = memory;
  result.size = file_size;
  return result;
}

static bool _asset_loader_hash_grow(AssetLoader *loader)
{
  ASSERT(0); // TODO
  return false;
}

static int32_t _asset_loader_hash_push(AssetLoader *loader, AssetNode *new_node)
{
  uint32_t start_idx = new_node->asset.id.hash & (loader->max_assets - 1);
  uint32_t idx = start_idx;

  while (true) {
    if (loader->assets[idx] == NULL) {
      loader->assets[idx] = new_node;
      return (int32_t) idx;
    }

    idx = (idx + 1) & (loader->max_assets - 1);
    if (idx == start_idx) {
      break;
    }
  }

  return -1;
}

static void _asset_loader_hash_remove(AssetLoader *loader, uint32_t index)
{
  loader->assets[index] = NULL;
}

static AssetNode *_asset_loader_hash_find(AssetLoader *loader, AssetId id)
{
  uint32_t start_idx = id.hash & (loader->max_assets - 1);
  uint32_t idx = start_idx;

  while (true) {
    AssetNode *node = loader->assets[idx];
    if (node == NULL) {
      break;
    }

    if (ASSET_IDS_EQUAL(node->asset.id, id)) {
      return node;
    }

    idx = (idx + 1) & (loader->max_assets - 1);
    if (idx == start_idx) {
      break;
    }
  }

  return NULL;
}

static void _asset_loader_list_insert_after(AssetLoader *loader, AssetNode *node, AssetNode *new_node)
{
  new_node->prev = node;
  new_node->next = node->next;

  new_node->next->prev = new_node;
  new_node->prev->next = new_node;
}

static void _asset_loader_list_remove(AssetLoader *loader, AssetNode *node)
{
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

static AssetNode *_asset_loader_lookup_asset(AssetLoader *loader, AssetId id)
{
  AssetNode *node = _asset_loader_hash_find(loader, id);
  if (node) {
    _asset_loader_list_remove(loader, node);
    _asset_loader_list_insert_after(loader, &loader->mru_asset, node);
  }

  return node;
}

static bool _asset_loader_push_asset(AssetLoader *loader, AssetNode *node)
{
   // TODO: Maybe grow hash
  int32_t index = _asset_loader_hash_push(loader, node);
  ASSERT(index >= 0);
  node->index = index;

  _asset_loader_list_insert_after(loader, &loader->mru_asset, node);
  loader->assets_count++;

  return true;
}

static Asset *asset_loader_get_dbc(AssetLoader *loader, char *name)
{
  AssetId id = loader->papi->get_asset_id(name);
  AssetNode *node = _asset_loader_lookup_asset(loader, id);
  if (node != NULL) {
    return &node->asset;
  }

  LoadedAsset asset_file = loader->papi->load_asset(name);
  if (asset_file.data == NULL) {
    return NULL;
  }

  node = ALLOCATE_ONE(&loader->allocator, AssetNode);
  node->asset.id = id;
  node->asset.type = AT_DBC;
  node->asset.dbc = (DBCFile *) asset_file.data;

  _asset_loader_push_asset(loader, node);

  return &node->asset;
}

static Asset *asset_loader_get_texture(AssetLoader *loader, char *name)
{
  AssetId id = loader->papi->get_asset_id(name);
  AssetNode *node = _asset_loader_lookup_asset(loader, id);
  if (node != NULL) {
    return &node->asset;
  }

  LoadedAsset asset_file = loader->papi->load_asset(name);
  if (asset_file.data == NULL) {
    return NULL;
  }

  BlpImage image;
  image.read_header(asset_file.data, asset_file.size);

  BlpHeader *header = &image.header;
  //printf("BLP %s\n%ux%u version %u, compression %u, alphaDepth %u, alphaType %u\n",
  //       name, header->width, header->height, header->version, header->compression,
  //       header->alphaDepth, header->alphaType);

  size_t data_size = sizeof(Texel) * header->width * header->height;
  size_t total_size = sizeof(AssetNode) + sizeof(Texture) + data_size;
  uint8_t *memory = (uint8_t*) ALLOCATE_SIZE(&loader->allocator, total_size);

  node = (AssetNode *) memory;
  node->asset.id = id;
  node->asset.type = AT_TEXTURE;

  Texture *texture = (Texture *) (memory + sizeof(AssetNode));
  node->asset.texture = texture;

  texture->width = header->width;
  texture->height = header->height;
  texture->pixels = (Texel *) (memory + sizeof(AssetNode) + sizeof(Texture));
  image.read_into_texture(asset_file.data, asset_file.size, texture);

  loader->papi->release_asset(&asset_file);

  _asset_loader_push_asset(loader, node);

  return &node->asset;
}

static Asset *asset_loader_get_model(AssetLoader *loader, char *name)
{
  AssetId id = loader->papi->get_asset_id(name);
  AssetNode *node = _asset_loader_lookup_asset(loader, id);
  if (node != NULL) {
    return &node->asset;
  }

  LoadedAsset asset_file = loader->papi->load_asset(name);
  if (asset_file.data == NULL) {
    return NULL;
  }

  node = ALLOCATE_ONE(&loader->allocator, AssetNode);
  node->asset.id = id;
  node->asset.type = AT_MODEL;
  node->asset.model = (M2Model *) m2_load(&loader->allocator, asset_file.data, asset_file.size);

  loader->papi->release_asset(&asset_file);

  _asset_loader_push_asset(loader, node);

  return &node->asset;
}

#ifdef WAV_FILE_SUPPORT
static Asset *asset_loader_get_wav(AssetLoader *loader, char *name)
{
  AssetId id = loader->papi->get_asset_id(name);
  AssetNode *node = _asset_loader_lookup_asset(loader, id);
  if (node != NULL) {
    return &node->asset;
  }

  LoadedAsset asset_file = loader->papi->load_asset(name);
  if (asset_file.data == NULL) {
    return NULL;
  }

  node = ALLOCATE_ONE(&loader->allocator, AssetNode);
  node->asset.id = id;
  node->asset.type = AT_WAV;
  node->asset.wav = ALLOCATE_ONE(&loader->allocator, WavFile);

  if (!wav_read(node->asset.wav, asset_file.data, asset_file.size)) {
    memory_free(&loader->allocator, node->asset.wav);
    memory_free(&loader->allocator, node);
    loader->papi->release_asset(&asset_file);
    return NULL;
  }

  _asset_loader_push_asset(loader, node);

  return &node->asset;
}
#endif

void asset_loader_init(AssetLoader *loader, PlatformAPI *papi)
{
  ASSERT(loader != NULL);

  loader->papi = papi;
  memory_allocator_init(&loader->allocator, papi);

  loader->max_assets = 1 << 9;
  loader->assets_count = 0;
  loader->assets = (AssetNode **) papi->allocate_memory(sizeof(AssetNode *) * loader->max_assets);

  loader->mru_asset = {};
  loader->mru_asset.prev = &loader->mru_asset;
  loader->mru_asset.next = &loader->mru_asset;
}
