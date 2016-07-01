/*
 * Copyright (c) 2016 CartoDB. All rights reserved.
 * Copying and using this code is allowed only according
 * to license terms, as given in https://cartodb.com/terms/
 */

#ifndef _CARTO_VECTORTILELAYER_H_
#define _CARTO_VECTORTILELAYER_H_

#include "layers/TileLayer.h"
#include "components/CancelableTask.h"
#include "components/Task.h"
#include "core/MapTile.h"
#include "vectortiles/VectorTileDecoder.h"

#include <memory>
#include <map>

#include <stdext/timed_lru_cache.h>

namespace carto {
    class TileDrawData;
    class TileRenderer;
        
    namespace VectorTileRenderOrder {
        /**
         * Vector tile rendering order.
         */
        enum VectorTileRenderOrder {
            /**
             * No rendering, elements are hidden.
             */
            VECTOR_TILE_RENDER_ORDER_HIDDEN = -1,
            /**
             * Elements are rendered together with the same layer elements.
             * Layers that are on top of the layers are rendered on top this layer.
             */
            VECTOR_TILE_RENDER_ORDER_LAYER = 0,
            /**
             * Elements are rendered on top of all normal layers.
             */
            VECTOR_TILE_RENDER_ORDER_LAST = 1
        };
    };
    
    /**
     * A tile layer where each tile is a bitmap. Should be used together with corresponding data source.
     */
    class VectorTileLayer : public TileLayer {
    public:
        /**
         * Constructs a VectorTileLayer object from a data source and tile decoder.
         * @param dataSource The data source from which this layer loads data.
         * @param decoder The tile decoder that decoder loaded tiles and applies styles.
         */
        VectorTileLayer(const std::shared_ptr<TileDataSource>& dataSource, const std::shared_ptr<VectorTileDecoder>& decoder);
        virtual ~VectorTileLayer();
    
        /**
         * Returns the tile decoder assigned to this layer.
         * @return The tile decoder assigned to this layer.
         */
        virtual std::shared_ptr<VectorTileDecoder> getTileDecoder() const;
        
        /**
         * Returns the tile cache capacity.
         * @return The tile cache capacity in bytes.
         */
        std::size_t getTileCacheCapacity() const;
        /**
         * Sets the vector tile cache capacity. Tile cache is the primary storage for vector data,
         * all tiles contained within the cache are stored as uncompressed vertex buffers and can immediately be
         * drawn to the screen. Setting the cache size too small may cause artifacts, such as disappearing tiles.
         * The more tiles are visible on the screen, the larger this cache should be. 
         * The default is 10MB, which should be enough for most use cases with preloading enabled. If preloading is
         * disabled, the cache size should be reduced by the user to conserve memory.
         * @return The new tile bitmap cache capacity in bytes.
         */
        void setTileCacheCapacity(std::size_t capacityInBytes);
        
        /**
         * Returns the current display order of the labels.
         * @return The display order of the labels. Default is VECTOR_TILE_RENDER_ORDER_LAYER.
         */
        VectorTileRenderOrder::VectorTileRenderOrder getLabelRenderOrder() const;
        /**
         * Sets the current display order of the labels.
         * @param renderOrder The new display order of the labels.
         */
        void setLabelRenderOrder(VectorTileRenderOrder::VectorTileRenderOrder renderOrder);
    
        /**
         * Returns the current display order of the buildings.
         * @return The display order of the buildigns. Default is VECTOR_TILE_RENDER_ORDER_LAYER.
         */
        VectorTileRenderOrder::VectorTileRenderOrder getBuildingRenderOrder() const;
        /**
         * Sets the current display order of the buildings.
         * @param renderOrder The new display order of the labels.
         */
        void setBuildingRenderOrder(VectorTileRenderOrder::VectorTileRenderOrder renderOrder);
    
    protected:
        virtual int getCullDelay() const;
    
        virtual bool tileExists(const MapTile& mapTile, bool preloadingCache) const;
        virtual bool tileValid(const MapTile& mapTile, bool preloadingCache) const;
        virtual void fetchTile(const MapTile& mapTile, bool preloadingTile, bool invalidated);
        virtual void clearTiles(bool preloadingTiles);
        virtual void tilesChanged(bool removeTiles);

        virtual long long getTileId(const MapTile& mapTile) const;

        virtual void calculateDrawData(const MapTile& visTile, const MapTile& closestTile, bool preloadingTile);
        virtual void refreshDrawData(const std::shared_ptr<CullState>& cullState);
    
        virtual int getMinZoom() const;
        virtual int getMaxZoom() const;
        
        virtual void offsetLayerHorizontally(double offset) ;
        
        virtual void onSurfaceCreated(const std::shared_ptr<ShaderManager>& shaderManager, const std::shared_ptr<TextureManager>& textureManager);
        virtual bool onDrawFrame(float deltaSeconds, BillboardSorter& billboardSorter, StyleTextureCache& styleCache, const ViewState& viewState);
        virtual bool onDrawFrame3D(float deltaSeconds, BillboardSorter& billboardSorter, StyleTextureCache& styleCache, const ViewState& viewState);
        virtual void onSurfaceDestroyed();
        
        virtual void registerDataSourceListener();
        virtual void unregisterDataSourceListener();

        // Configuration parameters that can be tweaked in subclasses
        bool _useFBO;
        bool _useDepth;
        bool _useStencil;
        bool _useTileMapMode;
    
    private:    
        class TileDecoderListener : public VectorTileDecoder::OnChangeListener {
        public:
            TileDecoderListener(const std::shared_ptr<VectorTileLayer>& layer);
            
            virtual void onDecoderChanged();
    
        private:
            std::weak_ptr<VectorTileLayer> _layer;
        };
    
        class FetchTask : public TileLayer::FetchTaskBase {
        public:
            FetchTask(const std::shared_ptr<VectorTileLayer>& layer, const MapTile& tile, bool preloadingTile);
            
        protected:
            virtual bool loadTile(const std::shared_ptr<TileLayer>& tileLayer);
        };
        
        class LabelCullTask : public CancelableTask {
        public:
            LabelCullTask(const std::shared_ptr<VectorTileLayer>& layer, const std::shared_ptr<TileRenderer>& renderer, const ViewState& viewState);
            
            virtual void cancel();
            virtual void run();
    
        private:
            std::weak_ptr<VectorTileLayer> _layer;
            std::weak_ptr<TileRenderer> _renderer;
            ViewState _viewState;
        };
    
        static const int CULL_DELAY_TIME = 200;
        static const int PRELOADING_PRIORITY_OFFSET = -2;
        static const int EXTRA_TILE_FOOTPRINT = 4096;
        static const int DEFAULT_PRELOADING_CACHE_SIZE = 10 * 1024 * 1024;
        
        VectorTileRenderOrder::VectorTileRenderOrder _labelRenderOrder;
        VectorTileRenderOrder::VectorTileRenderOrder _buildingRenderOrder;
    
        std::shared_ptr<VectorTileDecoder> _tileDecoder;
        std::shared_ptr<TileDecoderListener> _tileDecoderListener;

        std::shared_ptr<CancelableThreadPool> _labelCullThreadPool;
        std::shared_ptr<TileRenderer> _renderer;
    
        std::vector<std::shared_ptr<TileDrawData> > _tempDrawDatas;

        cache::timed_lru_cache<long long, std::shared_ptr<VectorTileDecoder::TileMap> > _visibleCache;
        cache::timed_lru_cache<long long, std::shared_ptr<VectorTileDecoder::TileMap> > _preloadingCache;
    };
    
}

#endif
