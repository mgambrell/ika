using System;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Drawing2D;

using Import;

using rho.Controls;

namespace rho
{

    class MapView : Form
    {
        TileSet tiles;
        Map map;
        int xwin,ywin;
        MainForm parent;
	
        StatusBar  statusbar;
        GraphView  graphview;
        ListView   layercontrol;

        private void Init(MainForm p,Map m,TileSet t)
        {
            parent=p;
            map=m;
            tiles=t;
		
            xwin=ywin=0;
		
            graphview=new GraphView();
            graphview.Dock=DockStyle.Fill;
            graphview.Redraw+=new PaintEventHandler(Redraw);
            Controls.Add(graphview);
            graphview.Show();
		
            graphview.OnHScroll+=new ScrollEventHandler(OnHScroll);
            graphview.OnVScroll+=new ScrollEventHandler(OnVScroll);

            statusbar=new StatusBar();
            statusbar.Dock=DockStyle.Bottom;
            Controls.Add(statusbar);
		
            Splitter splitter=new Splitter();
            splitter.Dock=DockStyle.Left;
            Controls.Add(splitter);
		
            layercontrol=new ListView();
            layercontrol.Dock=DockStyle.Left;
            layercontrol.Width=80;
            layercontrol.View=View.Details;
            Controls.Add(layercontrol);

            statusbar.SendToBack();	
				
            UpdateScrollBars();
		
            Resize+=new EventHandler(OnResize);
        }

        public MapView(MainForm p)
        {
            Map m=new Map(100,100);
            m.AddLayer();
            m.renderstring="1";

            TileSet t=new TileSet();
            t.AppendTile();

            Init(p,m,t);
        }
	
        public MapView(MainForm p,string filename) : base()
        {
            Map m=(Map)p.maps.Load(filename);
            TileSet t=(TileSet)p.tilesets.Load(m.vspname);

            Init(p,m,t);
        }
	
        void UpdateScrollBars()
        {
            graphview.AutoScrollMinSize=new Size(map.Width*tiles.Width,map.Height*tiles.Height);	
        }
	
        void OnHScroll(object o,ScrollEventArgs e)
        {
            if (e.Type==ScrollEventType.EndScroll)
                return;
				
            XWin=e.NewValue;
		
            graphview.Invalidate();
        }
	
        void OnVScroll(object o,ScrollEventArgs e)
        {
            if (e.Type==ScrollEventType.EndScroll)
                return;
		
            YWin=e.NewValue;

            graphview.Invalidate();
        }
	
        void OnResize(object o,EventArgs e)
        {
            UpdateScrollBars();

            // Need a way to find out how big the window was before now.
            // Then we have at most two skinny rects that need to be redrawn.
            //    graphview.Invalidate();
        }
	
        void Redraw(object o,PaintEventArgs e)
        {
            // Disable alpha blending for the first layer...
            e.Graphics.CompositingMode=CompositingMode.SourceCopy;
		
            int n=0;
		
            foreach (char c in map.renderstring)
            {
                int i=(int)c-'1';
			
                if (i>=0 && i<map.NumLayers)
                {
                    DrawLayer(map[i],e,n!=0);

                    if (n==0)
                        // ... and enable it for all subsequent layers
                        e.Graphics.CompositingMode=CompositingMode.SourceOver;

                    n++;
                }	
            }
        }
	
        void DrawLayer(Map.Layer layer,PaintEventArgs e,bool trans)
        {
            // parallax
            int xw=(int)(xwin*layer.parx);
            int yw=(int)(ywin*layer.pary);				
		
            // Only drawing the specified rect.  Dirty rectangles are fast. :D
            xw+=e.ClipRectangle.Left;
            yw+=e.ClipRectangle.Top;
		
            // first tile to be drawn
            int xtile= xw / tiles.Width;
            int ytile= yw / tiles.Height;
		
            // where to draw that first tile
            int xadj = - ( xw % tiles.Width  );
            int yadj = - ( yw % tiles.Height );
				
            // How many tiles to draw on each axis
            int xlen = e.ClipRectangle.Width / tiles.Width  + 2;
            int ylen = e.ClipRectangle.Height/ tiles.Height + 2;
		
            // Clip
            if (xtile+xlen>layer.Width)		xlen=layer.Width-xtile;
            if (ytile+ylen>layer.Height)	ylen=layer.Height-ytile;
		
            // now we loop, and draw
            int xt=xtile;	// need this later
            for (int y=0; y<ylen; y++)
            {
                for (int x=0; x<xlen; x++)
                {
                    int t=layer[xtile,ytile];
				
                    if (trans && t==0)
                    {
                        xtile++;
                        continue;
                    }
					
                    e.Graphics.DrawImage(tiles[t], x*tiles.Width+xadj, y*tiles.Height+yadj);
				
                    xtile++;
                }
                ytile++;
                xtile=xt;
            }
        }

        int XWin
        {
            get {   return xwin;    }
            set
            {
                xwin=value;
                if (xwin<0) xwin=0;
                if (xwin>map.Width*tiles.Width-graphview.ClientSize.Width)
                    xwin=map.Width*tiles.Width-graphview.ClientSize.Width;
            }
        }

        int YWin
        {
            get {   return ywin;    }
            set
            {
                ywin=value;
                if (ywin<0) ywin=0;
                if (ywin>map.Width*tiles.Width-graphview.ClientSize.Width)
                    ywin=map.Width*tiles.Width-graphview.ClientSize.Width;        }
        }
    }

}